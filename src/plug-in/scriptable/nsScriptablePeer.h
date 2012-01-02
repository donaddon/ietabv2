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

// This file is modified by Hong Jen Yee to used in IE Tab plug-in.

// ==============================
// ! Scriptability related code !
// ==============================
//
// nsScriptablePeer - xpconnect scriptable peer
//

#ifndef __nsScriptablePeer_h__
#define __nsScriptablePeer_h__

#include "nsIClassInfo.h"

class CWebBrowserCtrl;

// We must implement nsIClassInfo because it signals the
// Mozilla Security Manager to allow calls from JavaScript.

class nsClassInfoMixin : public nsIClassInfo
{
  // These flags are used by the DOM and security systems to signal that 
  // JavaScript callers are allowed to call this object's scritable methods.
  NS_IMETHOD GetFlags(PRUint32 *aFlags)
    {*aFlags = nsIClassInfo::PLUGIN_OBJECT | nsIClassInfo::DOM_OBJECT;
     return NS_OK;}
  NS_IMETHOD GetImplementationLanguage(PRUint32 *aImplementationLanguage)
    {*aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
     return NS_OK;}
  // The rest of the methods can safely return error codes...
  NS_IMETHOD GetInterfaces(PRUint32 *count, nsIID * **array)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetContractID(char * *aContractID)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetClassDescription(char * *aClassDescription)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetClassID(nsCID * *aClassID)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
    {return NS_ERROR_NOT_IMPLEMENTED;}
};

class nsScriptablePeer : public nsIIETabPlugin,
                         public nsClassInfoMixin
{
public:
  nsScriptablePeer();
  ~nsScriptablePeer();

public:
  // methods from nsISupports
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr); 
  NS_IMETHOD_(nsrefcnt) AddRef(); 
  NS_IMETHOD_(nsrefcnt) Release(); 

  void requestUpdateAll(){	if( requestTarget ) requestTarget->UpdateAll();	}
  void requestNewTab( const PRUnichar *url ){ if( requestTarget ) requestTarget->AddIeTab( url ); }
  void requestLoad( const PRUnichar *url ){	if( requestTarget ) requestTarget->LoadIeTab( url ); }
  void requestCloseWindow(){ canClose = true; if( requestTarget ) requestTarget->CloseIeTab(); }
  void requestProgressChange( long Progress ){ if( requestTarget ) requestTarget->OnProgressChange(Progress); }
  void requestSecurityChange( long Security ){ if( requestTarget ) requestTarget->OnSecurityChange(Security); }

protected: 
  nsrefcnt mRefCnt;

public:
  // native methods callable from JavaScript
  NS_DECL_NSIIETABPLUGIN

  void setBrowser( CWebBrowserCtrl* wb );

protected:
  CWebBrowserCtrl* m_pWebBrowser;
  nsIIeTab* requestTarget;
  bool canClose;

public:
  long progress;
  long security;
  bool ExecCmd(long id);
  void* nsMemDup(void* src, long size);
};

#endif