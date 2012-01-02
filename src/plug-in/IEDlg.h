#if !defined(AFX_IEDLG_H__9003C25E_F5EA_4E90_9871_DE71B9784A69__INCLUDED_)
#define AFX_IEDLG_H__9003C25E_F5EA_4E90_9871_DE71B9784A69__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IEDlg.h : header file

#include "stdafx.h"
#include "resource.h"
#include "WebBrowserCtrl.h"
#include "plugin.h"

//

#ifdef	_USRDLL
	#define DLLAPI	__declspec(dllexport)
#else
	#define DLLAPI	__declspec(dllimport)
#endif


class nsPluginInstance;

extern "C" DLLAPI HWND CreateIEWindow(HWND hparent, const char* URL, nsPluginInstance* plugin );

#define	WM_DESTROY_TEMP_IE	WM_APP + 15
#define	WM_IE_UPDATE_TITLE	WM_APP + 16
#define	WM_IE_UPDATE_STATUS	WM_APP + 17

/////////////////////////////////////////////////////////////////////////////
// CIEDlg dialog
class CWebBrowserCtrl;

class CIEDlg : public CDialog
{
// Construction
public:
	CString m_URL;
	CIEDlg( CWnd* pParent = NULL, 
		nsPluginInstance* PluginInst = NULL );   // standard constructor

	CWebBrowserCtrl* getWebBrowser(){	return &m_WB;	}
	CIETabScriptable* getScriptableObject() { return m_PluginInst->getScriptableObject(); }

	void setPluginInstance( nsPluginInstance* pi ){	m_PluginInst = pi;	}

// Dialog Data
	enum { IDD = IDD_IEFORM };
		// NOTE: the ClassWizard will add data members here

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CIEDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNewWindow2(LPDISPATCH FAR* ppDisp, BOOL FAR* Cancel);
	afx_msg void OnBeforeNavigate2(LPDISPATCH pDisp, VARIANT FAR* URL, VARIANT FAR* Flags, VARIANT FAR* TargetFrameName, VARIANT FAR* PostData, VARIANT FAR* Headers, BOOL FAR* Cancel);
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	LRESULT OnUpdateTitle(WPARAM wp, LPARAM lp);
	LRESULT OnUpdateStatus(WPARAM wp, LPARAM lp);
	void OnTitleChange(LPCTSTR Text);
	void OnCommandStateChange(long Command, BOOL Enable);
	void OnStatusChange(LPCTSTR Text);
	void OnProgressChange(long Progress, long ProgressMax);
	void OnDocumentCompleteIe(LPDISPATCH pDisp, VARIANT* URL);

	virtual void PostNcDestroy();

	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()
	//}}AFX_MSG

protected:
	CString m_StatusText;
	CWebBrowserCtrl m_WB;
	CWebBrowser2* m_TempIE;
	nsPluginInstance* m_PluginInst;
	UINT autoDestroyTimer;
	bool resizable;
	long width;
	long height;
public:
	void OnDownloadBegin();

	void startAutoDestroy(void)	{
		// If nobody wants to reuse this window, 
		// it will be automatically destroyed in 1 sec.
		autoDestroyTimer = SetTimer( 1, 20000, NULL );
	}

	void stopAutoDestroy(void)	{
		if( autoDestroyTimer )	{
			KillTimer( autoDestroyTimer );
			autoDestroyTimer = 0;
		}
	}
	afx_msg void OnTimer(UINT nIDEvent);
	void OnSetSecureLockIcon(long SecureLockIcon);
	void OnWindowClosing(BOOL IsChildWindow, BOOL* Cancel);

	virtual BOOL CreateControlSite(COleControlContainer* pContainer, 
				   COleControlSite** ppSite, UINT /* nID */, REFCLSID /* clsid */);

	afx_msg void OnDestroy();
//	virtual BOOL OnAmbientProperty(COleControlSite* pSite, DISPID dispid, VARIANT* pvar);
	void OnWindowSetResizable(BOOL Resizable);
	void OnWindowSetWidth(long Width);
	void OnWindowSetHeight(long Height);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IEDLG_H__9003C25E_F5EA_4E90_9871_DE71B9784A69__INCLUDED_)
