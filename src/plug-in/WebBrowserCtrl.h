#pragma once
#include "webbrowser2.h"
#include <afxcoll.h>

class CIEDlg;

class CWebBrowserCtrl :
	public CWebBrowser2
{
public:
	CWebBrowserCtrl(void);
	virtual ~CWebBrowserCtrl(void);
	bool GoBack(void);
	bool GoForward(void);
	bool isBackEnabled(){ return canBack; }
	bool isForwardEnabled(){ return canForward; }
	void enableBack(bool enable){ canBack = enable; }
	void enableForward(bool enable){ canForward = enable; }

protected:
	bool canBack;
	bool canForward;

public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
protected:
public:
	virtual BOOL DestroyWindow();
	CIEDlg *m_pDlg;
};

