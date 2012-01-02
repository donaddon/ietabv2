/////////////////////////////////////////////////////////////////////////////
// Name:        ScriptableObject.cpp
// Purpose:     npruntime scriptable support for IE Tab
// Author:      IETAB.NET http://www.ietab.net
// E-mail:      support@ietab.net
// Created:     2010.2.24
// Copyright:   (C) 2010 ietab.net
// License:     GPL : http://www.gnu.org/licenses/gpl.html
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "strsafe.h"
#include <npapi.h>
#include <npupp.h>
#include <npruntime.h>
#include <mshtmcid.h>
#include <mshtml.h>

#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"

#include "nsStringAPI.h"
#include "nsEmbedString.h"

#include <windowsx.h>

#include <wininet.h>

#include "ietab.h"
#include "scriptableobject.h"

#define SET_VAR_BOOL(npvariant, val) { npvariant->type = NPVariantType_Bool; npvariant->value.boolValue = val; }
#define SET_VAR_INT32(npvariant, val) { npvariant->type = NPVariantType_Int32; npvariant->value.intValue = val; }

#define STRZ_OUT_TO_NPVARIANT(pszIn, npvar) { \
	int nLen = strlen(pszIn); \
	char *pszOut = (char *) NPN_MemAlloc(nLen+1); \
	strcpy_s(pszOut, nLen+1, pszIn); \
	STRINGZ_TO_NPVARIANT(pszOut, npvar); }


_Ret_opt_z_cap_(nChars) inline LPSTR WINAPI W2Utf8Helper(_Out_z_cap_(nChars) LPSTR lpa, _In_z_ LPCWSTR lpw, _In_ int nChars) throw()
{
	ATLASSERT(lpw != NULL);
	ATLASSERT(lpa != NULL);
	if (lpa == NULL || lpw == NULL)
		return NULL;
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	*lpa = '\0';
	int ret = WideCharToMultiByte(CP_UTF8, 0, lpw, -1, lpa, nChars, NULL, NULL);
	if(ret == 0)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
	return lpa;
}

#define W2UTF8HELPER W2Utf8Helper

#define W2UTF8(lpw) (\
	((_lpw = lpw) == NULL) ? NULL : (\
		(_convert = (lstrlenW(_lpw)*4+1), \
		(_convert>INT_MAX/2) ? NULL : \
		W2UTF8HELPER((LPSTR) alloca(_convert*sizeof(WCHAR)), _lpw, _convert*sizeof(WCHAR)))))

extern "C" const IID CGID_IWebBrowser = 
	{0xed016940, 0xbd5b, 0x11cf,
	{0xba, 0x4e, 0x0, 0xc0, 0x4f, 0xd7, 0x08, 0x16}} ;

#define HTMLID_FIND			1
#define HTMLID_VIEWSOURCE	2


NPClass CIETabScriptable::MyClass =
{
	NP_CLASS_STRUCT_VERSION,
	CIETabScriptable::sAllocate,
	CIETabScriptable::sDeallocate,
	NULL,
	CIETabScriptable::sHasMethod,
	CIETabScriptable::sInvoke,
	CIETabScriptable::sInvokeDefault,
	CIETabScriptable::sHasProperty,
	CIETabScriptable::sGetProperty,
	CIETabScriptable::sSetProperty,
	CIETabScriptable::sRemoveProperty
};

static const char *arrszProperties[] =
{
	"requestTarget",
	"canClose",
	"canBack",
	"canForward",
	"canRefresh",
	"canStop",
	"canCut",
	"canCopy",
	"canPaste",
	"progress",
	"security",
	"url",
	"title"
};

static const char *arrszMethods[] =
{
	"goBack",
	"goForward",
	"navigate",
	"refresh",
	"stop",
	"saveAs",
	"print",
	"printPreview",
	"printSetup",
	"cut",
	"copy",
	"paste",
	"selectAll",
	"find",
	"viewSource",
	"focus"
};

// The scriptable object
CIETabScriptable::CIETabScriptable(NPP inst)
{
    m_pInstance = (nsPluginInstance *)inst->pdata;
	m_pWebBrowser = NULL;
	m_pRequestTarget = NULL;

	m_bCanClose = false;
	m_lProgress = -1;
	m_lSecurity = -1;
}

CIETabScriptable::~CIETabScriptable()
{
	if(m_pRequestTarget && (m_pRequestTarget->referenceCount > 0))
		NPN_ReleaseObject(m_pRequestTarget);
}

bool CIETabScriptable::ExecCmd(long id)
{
	if( m_pWebBrowser->GetSafeHwnd() && 
		m_pWebBrowser->QueryStatusWB( id ) & OLECMDF_ENABLED )
	{
		m_pWebBrowser->ExecWB( id, 0, NULL, NULL );
		return true;
	}
	return false;
}

// We use our own allocation routine so we can allocate our own IETab Object.
NPObject *CIETabScriptable::sAllocate(NPP inst, NPClass *pClass)
{
    return new CIETabScriptable(inst);
}

void CIETabScriptable::sDeallocate(NPObject *npobj)
{
	delete (CIETabScriptable *) npobj;
}

bool CIETabScriptable::sInvoke(NPObject* obj, NPIdentifier methodName,
				   const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	CIETabScriptable *pObj = (CIETabScriptable *) obj;
	return pObj->Invoke(methodName, args, argCount, result);
}

bool CIETabScriptable::sInvokeDefault(NPObject *obj, const NPVariant *args,
						  uint32_t argCount, NPVariant *result)
{
	CIETabScriptable *pObj = (CIETabScriptable *) obj;
	return pObj->InvokeDefault(args, argCount, result);
}

bool CIETabScriptable::sHasMethod(NPObject* obj, NPIdentifier methodName)
{
	CIETabScriptable *pObj = (CIETabScriptable *) obj;
	return pObj->HasMethod(methodName);
}

bool CIETabScriptable::sHasProperty(NPObject *obj, NPIdentifier propertyName)
{
	CIETabScriptable *pObj = (CIETabScriptable *) obj;
	return pObj->HasProperty(propertyName);
}

bool CIETabScriptable::sGetProperty(NPObject *obj, NPIdentifier propertyName,
						NPVariant *result)
{
	CIETabScriptable *pObj = (CIETabScriptable *) obj;
	return pObj->GetProperty(propertyName, result);
}

bool CIETabScriptable::sSetProperty(NPObject* obj, NPIdentifier propertyName,
                 const NPVariant *value)
{
	CIETabScriptable *pObj = (CIETabScriptable *) obj;
	return pObj->SetProperty(propertyName, value);
}

bool CIETabScriptable::sRemoveProperty(NPObject* obj, NPIdentifier propertyName)
{
	return false;
}

bool CIETabScriptable::Invoke(NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	char *name = NPN_UTF8FromIdentifier(methodName);
	if(!name)
		return false;

	if(!strcmp(name, "goBack"))
	{
		SET_VAR_BOOL(result, true);
		if( m_pWebBrowser->GetSafeHwnd() )
			m_pWebBrowser->GoBack();
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "goForward"))
	{
		SET_VAR_BOOL(result, true);
		if( m_pWebBrowser->GetSafeHwnd() )
			m_pWebBrowser->GoForward();
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "navigate"))
	{
		m_lProgress = -1;
		SET_VAR_BOOL(result, false);
		if(argCount>0 && m_pWebBrowser->GetSafeHwnd() )	{
			CComBSTR bstrUrl(args[0].value.stringValue.utf8characters);
			CComVariant vUrl = bstrUrl;
			try	{
				m_pWebBrowser->Navigate2( &vUrl, NULL, NULL, NULL, NULL );
				SET_VAR_BOOL(result, true);
			} catch(...) {}
		}
	}
	else if(!strcmp(name, "refresh"))
	{
		SET_VAR_BOOL(result, true);
		if( m_pWebBrowser->GetSafeHwnd() )
			m_pWebBrowser->Refresh();
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "stop"))
	{
		SET_VAR_BOOL(result, true);
		if( m_pWebBrowser->GetSafeHwnd() )
			m_pWebBrowser->Stop();
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "saveAs"))
	{
		SET_VAR_BOOL(result, ExecCmd( OLECMDID_SAVEAS ));
	}
	else if(!strcmp(name, "print"))
	{
		SET_VAR_BOOL(result, ExecCmd( OLECMDID_PRINT ));
	}
	else if(!strcmp(name, "printPreview"))
	{
		SET_VAR_BOOL(result, ExecCmd( OLECMDID_PRINTPREVIEW ));
	}
	else if(!strcmp(name, "printSetup"))
	{
		SET_VAR_BOOL(result, ExecCmd( OLECMDID_PRINT ));
	}
	else if(!strcmp(name, "cut"))
	{
		SET_VAR_BOOL(result, ExecCmd( OLECMDID_CUT ));
	}
	else if(!strcmp(name, "copy"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, !!(m_pWebBrowser->QueryStatusWB( OLECMDID_COPY ) & (OLECMDF_ENABLED|OLECMDF_ENABLED)))
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "paste"))
	{
		SET_VAR_BOOL(result, ExecCmd( OLECMDID_PASTE ));
	}
	else if(!strcmp(name, "selectAll"))
	{
		SET_VAR_BOOL(result, ExecCmd( OLECMDID_SELECTALL ));
	}
	else if(!strcmp(name, "find"))
	{
		SET_VAR_BOOL(result, false);
		if( m_pWebBrowser->GetSafeHwnd() )	{
			CComPtr<IDispatch> spDisp = m_pWebBrowser->get_Document();
			if(spDisp)
			{
				CComPtr<IOleCommandTarget> spTarget;
				spDisp->QueryInterface(IID_IOleCommandTarget, (void **)&spTarget);
				if(spTarget)
				{
					spTarget->Exec(&CGID_IWebBrowser, HTMLID_FIND, 0, NULL, NULL);
					SET_VAR_BOOL(result, true);
				}
			}
		}
	}
	else if(!strcmp(name, "viewSource"))
	{
		SET_VAR_BOOL(result, false);
		if( m_pWebBrowser->GetSafeHwnd() )	{
			CComPtr<IDispatch> spDisp = m_pWebBrowser->get_Document();
			if(spDisp)
			{
				CComPtr<IOleCommandTarget> spTarget;
				spDisp->QueryInterface(IID_IOleCommandTarget, (void **)&spTarget);
				if(spTarget)
				{
					spTarget->Exec(&CGID_IWebBrowser, HTMLID_VIEWSOURCE, 0, NULL, NULL);
					SET_VAR_BOOL(result, true);
				}
			}
		}
	}
	else if(!strcmp(name, "focus"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
		{
			CComPtr<IDispatch> spDisp = m_pWebBrowser->get_Document();
			if(spDisp)
			{
				CComPtr<IHTMLDocument2> spDoc2;
				spDisp->QueryInterface( &spDoc2 );
				if(spDoc2)
				{
					CComPtr<IHTMLWindow2> spWin2;
					spDoc2->get_parentWindow(&spWin2);
					if(spWin2)
					{
						spWin2->focus();
					}
				}
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool CIETabScriptable::InvokeDefault(const NPVariant *args,
						  uint32_t argCount, NPVariant *result)
{
	return true;
}

bool CIETabScriptable::HasMethod(NPIdentifier methodName)
{
    char *name = NPN_UTF8FromIdentifier(methodName);
	if(!name)
		return false;

	for(int i=0; i < ARRAYSIZE(arrszMethods); i++)
	{
	    if(!strcmp((const char *)name, arrszMethods[i]))
			return true;
	}
	return false;
}

bool CIETabScriptable::HasProperty(NPIdentifier propertyName)
{
    char *name = NPN_UTF8FromIdentifier(propertyName);
	if(!name)
		return false;

	for(int i=0; i < ARRAYSIZE(arrszProperties); i++)
	{
	    if(!strcmp((const char *)name, arrszProperties[i]))
			return true;
	}
	return false;
}

bool CIETabScriptable::GetProperty(NPIdentifier propertyName, NPVariant *result)
{
    char *name = NPN_UTF8FromIdentifier(propertyName);
	if(!name)
		return false;

	if(!strcmp(name, "requestTarget"))
	{
		if(m_pRequestTarget)
		{
			result->type = NPVariantType_Object;
			result->value.objectValue = m_pRequestTarget;
			NPN_RetainObject(m_pRequestTarget);
		}
		else
		{
			result->type = NPVariantType_Null;
		}
	}
	else if(!strcmp(name, "canClose"))
		SET_VAR_BOOL(result, m_bCanClose)
	else if(!strcmp(name, "canBack"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, m_pWebBrowser->isBackEnabled())
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "canForward"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, m_pWebBrowser->isForwardEnabled())
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "canRefresh"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, !!(m_pWebBrowser->QueryStatusWB( OLECMDID_REFRESH ) & (OLECMDF_ENABLED|OLECMDF_ENABLED)))
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "canStop"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, (m_lProgress != -1))
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "canCut"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, !!(m_pWebBrowser->QueryStatusWB( OLECMDID_CUT ) & (OLECMDF_ENABLED|OLECMDF_ENABLED)))
		else
			SET_VAR_BOOL(result, false);	
	}
	else if(!strcmp(name, "canCopy"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, !!(m_pWebBrowser->QueryStatusWB( OLECMDID_COPY ) & (OLECMDF_ENABLED|OLECMDF_ENABLED)))
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "canPaste"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_BOOL(result, !!(m_pWebBrowser->QueryStatusWB( OLECMDID_PASTE ) & (OLECMDF_ENABLED|OLECMDF_ENABLED)))
		else
			SET_VAR_BOOL(result, false);
	}
	else if(!strcmp(name, "progress"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_INT32(result, m_lProgress)
		else
			SET_VAR_INT32(result, -1);
	}
	else if(!strcmp(name, "security"))
	{
		if( m_pWebBrowser->GetSafeHwnd() )
			SET_VAR_INT32(result, m_lSecurity)
		else
			SET_VAR_INT32(result, -1);
	}
	else if(!strcmp(name, "url"))
	{
		result->type = NPVariantType_Null;
		if( m_pWebBrowser->GetSafeHwnd() ) {
			CComPtr<IDispatch> spApp = m_pWebBrowser->get_Application();
			if(spApp)
			{
				CComPtr<IWebBrowser2> spWB2;
				if( SUCCEEDED( spApp->QueryInterface(IID_IWebBrowser2, (void**)&spWB2 ) ) )
				{
					USES_CONVERSION;
					CComBSTR bstrUrl;
					if( SUCCEEDED( spWB2->get_LocationURL( &bstrUrl ) ) )	{
						char *pszUtf8 = W2UTF8(bstrUrl);
						STRZ_OUT_TO_NPVARIANT(pszUtf8, *result);
					}
				}
			}
		}
	}
	else if(!strcmp(name, "title"))
	{
		result->type = NPVariantType_Null;
		if( m_pWebBrowser->GetSafeHwnd() ) {
			CComPtr<IDispatch> spApp = m_pWebBrowser->get_Application();
			if(spApp)
			{
				CComPtr<IWebBrowser2> spWB2;
				if( SUCCEEDED( spApp->QueryInterface( &spWB2 )))
				{
					USES_CONVERSION;
					CComBSTR bstrUrl;
					if( SUCCEEDED( spWB2->get_LocationName( &bstrUrl ) ) )	{
						char *pszUtf8 = W2UTF8(bstrUrl);
						STRZ_OUT_TO_NPVARIANT(pszUtf8, *result);
					}
				}
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool CIETabScriptable::SetProperty(NPIdentifier propertyName, const NPVariant *value)
{
    char *name = NPN_UTF8FromIdentifier(propertyName);
	if(!name)
		return false;

	if(!strcmp(name, "requestTarget"))
	{
		if( m_pRequestTarget ) NPN_ReleaseObject( m_pRequestTarget );
		m_pRequestTarget = NPVARIANT_TO_OBJECT(*value);
		NPN_RetainObject(m_pRequestTarget);
	}
	else
	{
		return false;
	}

	return true;
}

// Notification methods

void CIETabScriptable::requestUpdateAll()
{
	if(!m_pInstance || !m_pRequestTarget)
		return;

    USES_CONVERSION;
    NPIdentifier methodName = NPN_GetStringIdentifier("updateAll");
    NPVariant arg, result;
	NPN_Invoke(m_pInstance->getInstance(), m_pRequestTarget, methodName, &arg, 0, &result);
}

void CIETabScriptable::requestGotFocus()
{
	if(!m_pInstance || !m_pRequestTarget)
		return;

    USES_CONVERSION;
    NPIdentifier methodName = NPN_GetStringIdentifier("gotFocus");
    NPVariant arg, result;
	NPN_Invoke(m_pInstance->getInstance(), m_pRequestTarget, methodName, &arg, 0, &result);
}

void CIETabScriptable::requestNewTab( LPCWSTR pwszUrl )
{
	if(!m_pInstance || !m_pRequestTarget)
		return;

    USES_CONVERSION;
    NPIdentifier methodName = NPN_GetStringIdentifier("addIeTab");
    NPVariant arg, result;
    STRINGZ_TO_NPVARIANT(W2UTF8(pwszUrl), arg);

	NPN_Invoke(m_pInstance->getInstance(), m_pRequestTarget, methodName, &arg, 1, &result);
}

void CIETabScriptable::requestLoad( LPCWSTR pwszUrl )
{
	if(!m_pInstance || !m_pRequestTarget)
		return;

    USES_CONVERSION;
    NPIdentifier methodName = NPN_GetStringIdentifier("loadIeTab");
    NPVariant arg, result;
    STRINGZ_TO_NPVARIANT(W2UTF8(pwszUrl), arg);

	NPN_Invoke(m_pInstance->getInstance(), m_pRequestTarget, methodName, &arg, 1, &result);
}

void CIETabScriptable::requestCloseWindow()
{
	if(!m_pInstance || !m_pRequestTarget)
		return;

    m_bCanClose = true;

	USES_CONVERSION;
    NPIdentifier methodName = NPN_GetStringIdentifier("closeIeTab");
    NPVariant arg, result;

	NPN_Invoke(m_pInstance->getInstance(), m_pRequestTarget, methodName, &arg, 0, &result);
}

void CIETabScriptable::requestProgressChange( long lProgress )
{
	if(!m_pInstance || !m_pRequestTarget)
		return;

    USES_CONVERSION;
    NPIdentifier methodName = NPN_GetStringIdentifier("onProgressChange");
    NPVariant arg, result;
	INT32_TO_NPVARIANT(lProgress, arg);

	NPN_Invoke(m_pInstance->getInstance(), m_pRequestTarget, methodName, &arg, 1, &result);
}

void CIETabScriptable::requestSecurityChange( long lSecurity )
{
	if(!m_pInstance || !m_pRequestTarget)
		return;

    USES_CONVERSION;
    NPIdentifier methodName = NPN_GetStringIdentifier("onSecurityChange");
    NPVariant arg, result;
	INT32_TO_NPVARIANT(lSecurity, arg);

	NPN_Invoke(m_pInstance->getInstance(), m_pRequestTarget, methodName, &arg, 1, &result);
}

