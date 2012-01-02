/////////////////////////////////////////////////////////////////////////////
// Name:        ietab.h
// Purpose:     Displey terminal screen data sroted in CTermData.
// Author:      PCMan (HZY)   http://pcman.ptt.cc/
// E-mail:      hzysoft@sina.com.tw
// Created:     2004.7.17
// Copyright:   (C) 2004 PCMan
// Licence:     GPL : http://www.gnu.org/licenses/gpl.html
// Modified by:
/////////////////////////////////////////////////////////////////////////////


#ifndef IETAB_H
#define IETAB_H

#include "stdafx.h"
#include "IEDlg.h"
#include <afxtempl.h>

class CWebBrowser2;

class CIETab
{
public:
	HWND m_hWnd;
	CIEDlg* m_pIEWnd;
	// class constructor
	CIETab(HWND hWnd, const char* url );
	// class destructor
	~CIETab();

	void init();

	CWebBrowser2* getWebBrowser(){	return m_pIEWnd->getWebBrowser();	}
protected:
	LRESULT WndProc(UINT msg, WPARAM wp, LPARAM lp);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	WNDPROC m_OldWndProc;
	void OnSize( int SizeType, int cx, int cy );
	CString m_URL;

	static CList<CIEDlg*> browserPool;
public:
	void destroy(void);
	static void cleanupPool(void);
	static CIEDlg* popFromPool(void);
	static void pushToPool(CIEDlg* dlg);

};

#endif // IETAB_H

