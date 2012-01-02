/////////////////////////////////////////////////////////////////////////////
// Name:        ScriptableObject.h
// Purpose:     npruntime scriptable support for IE Tab
// Author:      IETAB.NET http://www.ietab.net
// E-mail:      support@ietab.net
// Created:     2010.2.24
// Copyright:   (C) 2010 ietab.net
// License:     GPL : http://www.gnu.org/licenses/gpl.html
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include <nscore.h>
#include "plugin.h"


class CIETabScriptable : public NPObject
{
public:
    CIETabScriptable(NPP inst);
    ~CIETabScriptable();

	// NPP methods
	bool Invoke(NPIdentifier methodName,
				   const NPVariant *args, uint32_t argCount, NPVariant *result);
	bool InvokeDefault(const NPVariant *args,
						  uint32_t argCount, NPVariant *result);
	bool HasMethod(NPIdentifier methodName);
	bool HasProperty(NPIdentifier propertyName);
	bool GetProperty(NPIdentifier propertyName, NPVariant *result);
	bool SetProperty(NPIdentifier propertyName, const NPVariant *value);
	bool RemoveProperty(NPIdentifier propertyName);
	
	// Static callback methods.
	static NPObject *sAllocate(NPP inst, NPClass *);
	static void sDeallocate(NPObject *npobj);
	static bool sInvoke(NPObject* obj, NPIdentifier methodName,
				   const NPVariant *args, uint32_t argCount, NPVariant *result);
	static bool sInvokeDefault(NPObject *obj, const NPVariant *args,
						  uint32_t argCount, NPVariant *result);
	static bool sHasMethod(NPObject* obj, NPIdentifier methodName);
	static bool sHasProperty(NPObject *obj, NPIdentifier propertyName);
	static bool sGetProperty(NPObject *obj, NPIdentifier propertyName, NPVariant *result);
	static bool sSetProperty(NPObject* obj, NPIdentifier propertyName,
                     const NPVariant *value);
	static bool sRemoveProperty(NPObject* obj, NPIdentifier propertyName);

	static NPClass MyClass;

	void setBrowser( CWebBrowserCtrl* wb ) { m_pWebBrowser = wb; };

	// Notification methods
	void requestUpdateAll();
	void requestNewTab( const PRUnichar *url );
	void requestLoad( const PRUnichar *url );
	void requestCloseWindow();
	void requestProgressChange( long Progress );
	void requestSecurityChange( long Security );
	void requestGotFocus();

protected:
	bool ExecCmd(long id);

protected:
    nsPluginInstance *m_pInstance;
	CWebBrowserCtrl* m_pWebBrowser;
    NPObject *m_pRequestTarget;

public:
	bool m_bCanClose;
	long m_lProgress;
	long m_lSecurity;
};
