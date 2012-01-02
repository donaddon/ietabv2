/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// ==============================
// ! Scriptability related code !
// ==============================

/////////////////////////////////////////////////////
//
// This file implements the nsScriptablePeer object
// The native methods of this class are supposed to
// be callable from JavaScript
//

#include "stdafx.h"

#include "npapi.h"
#include <mshtmcid.h>
#include <mshtml.h>

#include "WebBrowserCtrl.h"
#include ".\nsscriptablepeer.h"

#include "nsXPCOM.h"
#include "nsIMemory.h"

static NS_DEFINE_IID(kIIETabPluginIID, NS_IIETABPLUGIN_IID);
static NS_DEFINE_IID(kIClassInfoIID, NS_ICLASSINFO_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

nsScriptablePeer::nsScriptablePeer()
	: m_pWebBrowser(NULL), mRefCnt(0)
	, requestTarget(NULL)
	, canClose( false )
	, progress( -1 )
	, security( -1 )
{
}

nsScriptablePeer::~nsScriptablePeer()
{
	if( requestTarget ) NS_RELEASE( requestTarget );
}

// AddRef, Release and QueryInterface are common methods and must 
// be implemented for any interface
NS_IMETHODIMP_(nsrefcnt) nsScriptablePeer::AddRef() 
{ 
	++mRefCnt; 
	return mRefCnt; 
} 

NS_IMETHODIMP_(nsrefcnt) nsScriptablePeer::Release() 
{ 
	--mRefCnt; 
	if (mRefCnt == 0) { 
		delete this;
		return 0; 
	} 
	return mRefCnt; 
} 

// here nsScriptablePeer should return three interfaces it can be asked for by their iid's
// static casts are necessary to ensure that correct pointer is returned
NS_IMETHODIMP nsScriptablePeer::QueryInterface(const nsIID& aIID, void** aInstancePtr) 
{ 
	if(!aInstancePtr) 
		return NS_ERROR_NULL_POINTER; 

	if(aIID.Equals(kIIETabPluginIID)) {
		*aInstancePtr = static_cast<nsIIETabPlugin*>(this); 
		AddRef();
		return NS_OK;
	}

	if(aIID.Equals(kIClassInfoIID)) {
		*aInstancePtr = static_cast<nsIClassInfo*>(this); 
		AddRef();
		return NS_OK;
	}

	if(aIID.Equals(kISupportsIID)) {
		*aInstancePtr = static_cast<nsISupports*>(static_cast<nsIIETabPlugin*>(this)); 
		AddRef();
		return NS_OK;
	}

	return NS_NOINTERFACE; 
}

void nsScriptablePeer::setBrowser( CWebBrowserCtrl* wb )
{
	m_pWebBrowser = wb;
}

//
// the following methods will be callable from JavaScript
//

/* boolean navigate (in wstring url); */
NS_IMETHODIMP nsScriptablePeer::Navigate(const PRUnichar *url, PRBool *_retval)
{
	progress = -1;
	*_retval = false;
	if( m_pWebBrowser->GetSafeHwnd() )	{
		BSTR bstrUrl = ::SysAllocString( url );
		VARIANT vUrl; VariantInit( &vUrl );	vUrl.vt = VT_BSTR; vUrl.bstrVal = bstrUrl;
		try	{
			m_pWebBrowser->Navigate2( &vUrl, NULL, NULL, NULL, NULL );
			*_retval = true;
		} catch(...) {}
		VariantClear( &vUrl );
		::SysFreeString( bstrUrl );
	}
	return NS_OK;
}


/* readonly attribute boolean canClose; */
NS_IMETHODIMP nsScriptablePeer::GetCanClose(PRBool *aCanBack)
{
	*aCanBack = canClose;
	return NS_OK;
}


/* readonly attribute boolean canBack; */
NS_IMETHODIMP nsScriptablePeer::GetCanBack(PRBool *aCanBack)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aCanBack = m_pWebBrowser->isBackEnabled();
	else
		*aCanBack = false;
	return NS_OK;
}

/* boolean goBack (); */
NS_IMETHODIMP nsScriptablePeer::GoBack(PRBool *_retval)
{
	*_retval = true;
	if( m_pWebBrowser->GetSafeHwnd() )
		m_pWebBrowser->GoBack();
	else
		*_retval = false;
	return NS_OK;
}

/* readonly attribute boolean canForward; */
NS_IMETHODIMP nsScriptablePeer::GetCanForward(PRBool *aCanForward)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aCanForward = m_pWebBrowser->isForwardEnabled();
	else
		*aCanForward = false;
	return NS_OK;
}

/* boolean goForward (); */
NS_IMETHODIMP nsScriptablePeer::GoForward(PRBool *_retval)
{
	*_retval = true;
	if( m_pWebBrowser->GetSafeHwnd() )
		m_pWebBrowser->GoForward();
	else
		*_retval = false;
	return NS_OK;
}

/* readonly attribute boolean canRefresh; */
NS_IMETHODIMP nsScriptablePeer::GetCanRefresh(PRBool *aCanRefresh)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aCanRefresh = !!(m_pWebBrowser->QueryStatusWB( OLECMDID_REFRESH ) & (OLECMDF_ENABLED|OLECMDF_ENABLED));
	else
		*aCanRefresh = false;
	return NS_OK;
}

/* boolean refresh (); */
NS_IMETHODIMP nsScriptablePeer::Refresh(PRBool *_retval)
{
	*_retval = true;
	if( m_pWebBrowser->GetSafeHwnd() )
		m_pWebBrowser->Refresh();
	else
		*_retval = false;
	return NS_OK;
}

/* readonly attribute boolean canStop; */
NS_IMETHODIMP nsScriptablePeer::GetCanStop(PRBool *aCanStop)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		//*aCanStop = !!(m_pWebBrowser->QueryStatusWB( OLECMDID_STOP ) & (OLECMDF_ENABLED|OLECMDF_ENABLED));
		*aCanStop = (progress != -1);
	else
		*aCanStop = false;
	return NS_OK;
}


/* boolean stop (); */
NS_IMETHODIMP nsScriptablePeer::Stop(PRBool *_retval)
{
	*_retval = true;
	if( m_pWebBrowser->GetSafeHwnd() )
		m_pWebBrowser->Stop();
	else
		*_retval = false;
	return NS_OK;
}

/* readonly attribute long progress; */
NS_IMETHODIMP nsScriptablePeer::GetProgress(PRInt32 *aProgress)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aProgress = progress;
	else
		*aProgress = -1;
	return NS_OK;
}

/* readonly attribute long security; */
NS_IMETHODIMP nsScriptablePeer::GetSecurity(PRInt32 *aSecurity)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aSecurity = security;
	else
		*aSecurity = -1;
	return NS_OK;
}

/* readonly attribute wstring url; */
NS_IMETHODIMP nsScriptablePeer::GetUrl(PRUnichar * *aUrl)
{
	*aUrl = NULL;
	if( m_pWebBrowser->GetSafeHwnd() ) {
		IDispatch* app = m_pWebBrowser->get_Application();
		if( app )	{
			IWebBrowser2* wb = NULL;
			if( SUCCEEDED( app->QueryInterface(IID_IWebBrowser2, (void**)&wb ) ) ) {
				BSTR bstrUrl = NULL;
				if( SUCCEEDED( wb->get_LocationURL( &bstrUrl ) ) )	{
					UINT len = ::SysStringLen( bstrUrl ) + 1;
					*aUrl = (PRUnichar*)nsMemDup( bstrUrl, sizeof(WCHAR) * len );
					::SysFreeString( bstrUrl );
				}
				wb->Release();
			}
			app->Release();
		}
	}
	return NS_OK; //*aUrl ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* readonly attribute wstring title; */
NS_IMETHODIMP nsScriptablePeer::GetTitle(PRUnichar * *aTitle)
{
	*aTitle = NULL;
	if( m_pWebBrowser->GetSafeHwnd() ) {
		IDispatch* app = m_pWebBrowser->get_Application();
		if( app )	{
			IWebBrowser2* wb = NULL;
			if( SUCCEEDED( app->QueryInterface(IID_IWebBrowser2, (void**)&wb ) ) ) {
				BSTR bstrTitle = NULL;
				if( SUCCEEDED( wb->get_LocationName( &bstrTitle ) ) )	{
					UINT len = ::SysStringLen( bstrTitle ) + 1;
					*aTitle = (PRUnichar*)nsMemDup( bstrTitle, sizeof(WCHAR) * len );
					::SysFreeString( bstrTitle );
				}
				wb->Release();
			}
			app->Release();
		}
	}	
	return NS_OK; //*aTitle ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* boolean saveAs (); */
NS_IMETHODIMP nsScriptablePeer::SaveAs(PRBool *_retval)
{
	*_retval = ExecCmd( OLECMDID_SAVEAS );
	return NS_OK;
}

/* boolean print (); */
NS_IMETHODIMP nsScriptablePeer::Print(PRBool *_retval)
{
	*_retval = ExecCmd( OLECMDID_PRINT );
	return NS_OK;
}

/* boolean printPreview (); */
NS_IMETHODIMP nsScriptablePeer::PrintPreview(PRBool *_retval)
{
	*_retval = ExecCmd( OLECMDID_PRINTPREVIEW/*IDM_PRINTPREVIEW*/ );
	return NS_OK;
}

/* boolean printSetup (); */
NS_IMETHODIMP nsScriptablePeer::PrintSetup(PRBool *_retval)
{
	*_retval = ExecCmd( OLECMDID_PRINT );
	return NS_OK;
}

/* readonly attribute boolean canCut; */
NS_IMETHODIMP nsScriptablePeer::GetCanCut(PRBool *aCanCut)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aCanCut = !!(m_pWebBrowser->QueryStatusWB( OLECMDID_CUT ) & (OLECMDF_ENABLED|OLECMDF_ENABLED));
	else
		*aCanCut = false;
	return NS_OK;
}

/* boolean cut (); */
NS_IMETHODIMP nsScriptablePeer::Cut(PRBool *_retval)
{
	*_retval = ExecCmd( OLECMDID_CUT );
	return NS_OK;
}

/* readonly attribute boolean canCopy; */
NS_IMETHODIMP nsScriptablePeer::GetCanCopy(PRBool *aCanCopy)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aCanCopy = !!(m_pWebBrowser->QueryStatusWB( OLECMDID_COPY ) & (OLECMDF_ENABLED|OLECMDF_ENABLED));
	else
		*aCanCopy = false;
	return NS_OK;
}

/* boolean copy (); */
NS_IMETHODIMP nsScriptablePeer::Copy(PRBool *_retval)
{
	*_retval = ExecCmd( OLECMDID_COPY );
	return NS_OK;
}

/* readonly attribute boolean canPaste; */
NS_IMETHODIMP nsScriptablePeer::GetCanPaste(PRBool *aCanPaste)
{
	if( m_pWebBrowser->GetSafeHwnd() )
		*aCanPaste = !!(m_pWebBrowser->QueryStatusWB( OLECMDID_PASTE ) & (OLECMDF_ENABLED|OLECMDF_ENABLED));
	else
		*aCanPaste = false;
	return NS_OK;
}


/* boolean paste (); */
NS_IMETHODIMP nsScriptablePeer::Paste(PRBool *_retval)
{
	*_retval = 	*_retval = ExecCmd( OLECMDID_PASTE );
	return NS_OK;
}

/* boolean selectAll (); */
NS_IMETHODIMP nsScriptablePeer::SelectAll(PRBool *_retval)
{
	*_retval = ExecCmd( OLECMDID_SELECTALL );
	return NS_OK;
}

#define HTMLID_FIND			1
#define HTMLID_VIEWSOURCE	2

extern "C" const IID CGID_IWebBrowser;

/* boolean find (); */
NS_IMETHODIMP nsScriptablePeer::Find(PRBool *_retval)
{
	*_retval = false;
	if( m_pWebBrowser->GetSafeHwnd() )	{
		LPDISPATCH lpd = m_pWebBrowser->get_Document();
		if(lpd)	{
			IOleCommandTarget* pcmd=NULL;
			if( SUCCEEDED(lpd->QueryInterface(IID_IOleCommandTarget,(void**)&pcmd) ) )
			{
				pcmd->Exec(&CGID_IWebBrowser, HTMLID_FIND, 0, NULL, NULL);
				pcmd->Release();
				*_retval = true;
			}
			lpd->Release();
		}
	}
	return NS_OK;
}

/* boolean viewSource (); */
NS_IMETHODIMP nsScriptablePeer::ViewSource(PRBool *_retval)
{
	*_retval = false;
	if( m_pWebBrowser->GetSafeHwnd() )	{
		LPDISPATCH lpd = m_pWebBrowser->get_Document();
		if(lpd)	{
			IOleCommandTarget* pcmd=NULL;
			if( SUCCEEDED(lpd->QueryInterface(IID_IOleCommandTarget,(void**)&pcmd) ) )
			{
				pcmd->Exec(&CGID_IWebBrowser, HTMLID_VIEWSOURCE, 0, NULL, NULL);
				pcmd->Release();
				*_retval = true;
			}
			lpd->Release();
		}
	}
	return NS_OK;
}

/* boolean focus (); */
NS_IMETHODIMP nsScriptablePeer::Focus()
{
	if( m_pWebBrowser->GetSafeHwnd() )	{
		IDispatch* doc = m_pWebBrowser->get_Document();
		if( doc ) {
			IHTMLDocument2* doc2 = NULL;
			if( SUCCEEDED( doc->QueryInterface( IID_IHTMLDocument2, (void**)&doc2 ) ) ) {
				IHTMLWindow2* wnd = NULL;
				if( SUCCEEDED( doc2->get_parentWindow( &wnd ) ) ) {
					wnd->focus();
					wnd->Release();
				}
				doc2->Release();
			}
			doc->Release();
		}
	}
	return NS_OK;
}


bool nsScriptablePeer::ExecCmd(long id)
{
	if( m_pWebBrowser->GetSafeHwnd() && 
		m_pWebBrowser->QueryStatusWB( id ) & OLECMDF_ENABLED )
	{
		m_pWebBrowser->ExecWB( id, 0, NULL, NULL );
		return true;
	}
	return false;
}

void* nsScriptablePeer::nsMemDup(void* src, long size)
{
	nsIMemory* mem = NULL;
	if( NS_OK == NS_GetMemoryManager( &mem ) )
	{
		char* dest = (char*)mem->Alloc( size );
		mem->Release();
		if( dest )	{
			memcpy( dest, src, size );
			return dest;
		}
	}
	return NULL;
}


NS_IMETHODIMP nsScriptablePeer::GetRequestTarget(nsISupports * *aRequestTarget)
{
	/*
	if( requestTarget ) {
		requestTarget->QueryInterface( NS_GET_IID(nsISupports), (void**)&aRequestTarget );
	}
	else {
		*aRequestTarget = NULL;
	}
	*/
	return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::SetRequestTarget(nsISupports * aRequestTarget)
{
	if( requestTarget )	NS_RELEASE( requestTarget );
	
	if( aRequestTarget ) {
		aRequestTarget->QueryInterface( NS_GET_IID(nsIIeTab), (void**)&requestTarget );
	}
/*
	if( NS_SUCCEEDED( aRequestTarget->QueryInterface( NS_GET_IID(nsIIeTab), (void**)&requestTarget ) ) )
	{
		NS_ADDREF( requestTarget );
	}
*/
	return NS_OK;
}

