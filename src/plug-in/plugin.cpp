/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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


#include "stdafx.h"

#include "npapi.h"

#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"

#include "nsStringAPI.h"
#include "nsEmbedString.h"

#include <windowsx.h>

#include <wininet.h>

#include "plugin.h"
#include "ietab.h"
#include "ScriptableObject.h"
#include "icbcworkaround.h"

//////////////////////////////////////
//
// general initialization and shutdown
//
NPError NS_PluginInitialize()
{
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

/////////////////////////////////////////////////////////////
//
// construction and destruction of our plugin instance object
//
nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct);
  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstance *)aPlugin;
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(nsPluginCreateData* aCreateDataStruct) : nsPluginInstanceBase(),
  mInstance(aCreateDataStruct->instance), mInitialized(FALSE),
  securityCheckOK( false )
{
   mhWnd = NULL;
   m_pIETab = NULL;
   m_pIETabScriptable = NULL;
   m_URL = NULL;

   InitICBCWorkaround();

   if( aCreateDataStruct->mode==NP_EMBED )
   {
      for( int i=0; i < aCreateDataStruct->argc; ++i )
      {
         if( 0 == _strcmpi( "URL", aCreateDataStruct->argn[i] ) )
            m_URL = aCreateDataStruct->argv[i];
      }
   }

}

nsPluginInstance::~nsPluginInstance()
{
   UninitICBCWorkaround();

   //
   // m_pIETabScriptable is also held by the browser, so 
   // releasing it here does not guarantee that it is over
   // we should take precaution in case it will be called later
   // and zero its mPlugin member
	if(m_pIETabScriptable)
	{
		m_pIETabScriptable->setBrowser(NULL);
		NPN_ReleaseObject(m_pIETabScriptable);
	}
}

CString GetDOMUrl(NPP instance)
{
	CString strUrl;
	NPVariant objLocation;
	NPVariant npstrHref;
	NPIdentifier idHref = NPN_GetStringIdentifier("href");
	NPIdentifier idLocation = NPN_GetStringIdentifier("location");
	NPObject *pobjWin = NULL;
	NPObject *pobjLoc = NULL;

	VOID_TO_NPVARIANT(objLocation);
	VOID_TO_NPVARIANT(npstrHref);
	do
	{
		if( NPN_GetValue(instance, NPNVWindowNPObject, &pobjWin) != NPERR_NO_ERROR )
			break;
		if( !NPN_GetProperty(instance, pobjWin, idLocation, &objLocation) || !NPVARIANT_IS_OBJECT(objLocation))
			break;
	
		pobjLoc = NPVARIANT_TO_OBJECT(objLocation);
		if(!NPN_GetProperty(instance, pobjLoc, idHref, &npstrHref) || !NPVARIANT_IS_STRING(npstrHref))
			break;

		strUrl = CString(NPVARIANT_TO_STRING(npstrHref).utf8characters,  NPVARIANT_TO_STRING(npstrHref).utf8length);
	} while (0);

	NPN_ReleaseVariantValue(&objLocation);
	NPN_ReleaseVariantValue(&npstrHref);

	return strUrl;
}

#define GWL_USERDATA        (-21)

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
	::OleInitialize(NULL);
	if(aWindow == NULL)
    return FALSE;

  mhWnd = (HWND)aWindow->window;
  if(mhWnd == NULL)
    return FALSE;

  SetWindowLong(mhWnd, GWL_USERDATA, (LONG)this);

  // Security check: Only URLs started with chrome:// are allowed to use this plug-in
  CString strUrl = GetDOMUrl( this->getInstance() );
  if(strUrl.Find("chrome:") == 0)
  {
	   m_pIETab = new CIETab(mhWnd, m_URL);
	   m_pIETab->init();

	   // associate window with our nsPluginInstance object so we can access
	   // it in the window procedure
	   SetWindowLong(mhWnd, GWL_STYLE, GetWindowLong(mhWnd, GWL_STYLE)|WS_CLIPCHILDREN);
	   SetWindowLong(mhWnd, GWL_EXSTYLE, GetWindowLong(mhWnd, GWL_EXSTYLE)|WS_EX_CONTROLPARENT);
  }
  mInitialized = TRUE;

  return TRUE;
}

void nsPluginInstance::shut()
{
  if( m_pIETab )
  {
    m_pIETab->destroy();
    delete m_pIETab;
  }

  mhWnd = NULL;
  mInitialized = FALSE;
}

NPBool nsPluginInstance::isInitialized()
{
  return mInitialized;
}

// ==============================
// ! Scriptability related code !
// ==============================
//

void nsPluginInstance::CreateScriptableObject()
{
	NPObject *so = NPN_CreateObject(getInstance(), &CIETabScriptable::MyClass);
	m_pIETabScriptable = (CIETabScriptable *) so;

	// We retain it until we are released ourselves.
	NPN_RetainObject(so);
}

NPError  nsPluginInstance::GetValue(NPPVariable aVariable, void *aValue)
{
	switch(aVariable) {
	default:
		return NPERR_GENERIC_ERROR;
	case NPPVpluginNameString:
		*((char **)aValue) = "IE Tab 2";
		break;
	case NPPVpluginDescriptionString:
		*((char **)aValue) = "IE Tab 2 plugin.";
		break;
	case NPPVpluginScriptableNPObject:
        {
			NPObject *so = (NPObject *) m_pIETabScriptable;

			if(!so)
			{
				CreateScriptableObject();
				so = (NPObject *) m_pIETabScriptable;
			}

			if(so)
			{
				NPN_RetainObject(so);
			}

			*(NPObject **)aValue = so;
        }
		break;
#if defined(XULRUNNER_SDK)
	case NPPVpluginNeedsXEmbed:
		*((PRBool *)value) = PR_FALSE;
		break;
#endif
	}
	return NPERR_NO_ERROR;
}

// ==============================
// ! Scriptability related code !
// ==============================
//

CIETabScriptable* nsPluginInstance::getScriptableObject()
{
	if(!m_pIETabScriptable)
	{
		CreateScriptableObject();
	}
	return m_pIETabScriptable;
}


const char * nsPluginInstance::getVersion()
{
  return NPN_UserAgent(mInstance);
}

CWinApp theApp;
