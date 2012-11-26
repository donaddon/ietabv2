// IEDlg.cpp : implementation file
//

#include "stdafx.h"

#include "nsCOMPtr.h"
//#include "nsIDOMWindow.h"
//#include "nsIDOMDocument.h"
//#include "nsEmbedString.h"

#include <windowsx.h>
#include "IETab.h"
#include "IEDlg.h"
#include "WebBrowser2.h"
#include ".\iedlg.h"

#include "npapi.h"
#include "plugin.h"

#include "ScriptableObject.h"

#include <exdispid.h>
#include <mshtmcid.h>
#include <mshtmhst.h>
#include <mshtmdid.h>
#include <mshtml.h>
#include <afxocc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIEDlg dialog


CIEDlg::CIEDlg( CWnd* pParent /*=NULL*/, nsPluginInstance* PluginInst )
	: CDialog(CIEDlg::IDD, pParent), m_PluginInst(PluginInst), m_TempIE(NULL)
	, m_StatusText(_T("")), resizable(true), width(0), height(0)
{
	m_WB.m_pDlg = this;
	//{{AFX_DATA_INIT(CIEDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


BEGIN_MESSAGE_MAP(CIEDlg, CDialog)
	//{{AFX_MSG_MAP(CIEDlg)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
	ON_MESSAGE( WM_IE_UPDATE_TITLE, OnUpdateTitle )
	ON_MESSAGE( WM_IE_UPDATE_STATUS, OnUpdateStatus )

	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIEDlg message handlers

void CIEDlg::OnSize(UINT nType, int cx, int cy) 
{
	if( m_WB.m_hWnd ) {
		if( !resizable && (width || height) ) {
			m_WB.SetWindowPos(NULL, 0, 0,
				width, height, SWP_NOACTIVATE | SWP_NOZORDER);
		}
		else {
			m_WB.SetWindowPos(NULL, 0, 0,
				cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
		}
	}
}

BEGIN_EVENTSINK_MAP(CIEDlg, CDialog)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_NEWWINDOW2, OnNewWindow2, VTS_PDISPATCH VTS_PBOOL)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_BEFORENAVIGATE2, OnBeforeNavigate2, VTS_DISPATCH VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PBOOL)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_TITLECHANGE, OnTitleChange, VTS_BSTR)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_COMMANDSTATECHANGE, OnCommandStateChange, VTS_I4 VTS_BOOL)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_STATUSTEXTCHANGE, OnStatusChange, VTS_BSTR)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_PROGRESSCHANGE, OnProgressChange, VTS_I4 VTS_I4)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_DOCUMENTCOMPLETE, OnDocumentCompleteIe, VTS_DISPATCH VTS_PVARIANT)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_DOWNLOADBEGIN, OnDownloadBegin, VTS_NONE)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_SETSECURELOCKICON, OnSetSecureLockIcon, VTS_I4)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_WINDOWCLOSING, OnWindowClosing, VTS_BOOL VTS_PBOOL)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_WINDOWSETRESIZABLE, OnWindowSetResizable, VTS_BOOL)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_WINDOWSETWIDTH, OnWindowSetWidth, VTS_I4)
	ON_EVENT(CIEDlg, IDC_IE, DISPID_WINDOWSETHEIGHT, OnWindowSetHeight, VTS_I4)
END_EVENTSINK_MAP()

class CMyCriticalSection
{
public:
	CMyCriticalSection()	{
		InitializeCriticalSection( &cs );
	}

	~CMyCriticalSection()	{
		DeleteCriticalSection( &cs );
	}
	void enter(){	EnterCriticalSection( &cs );	}
	void leave(){	LeaveCriticalSection( &cs );	}
private:
	CRITICAL_SECTION cs;
};

void CIEDlg::OnNewWindow2(LPDISPATCH FAR* ppDisp, BOOL FAR* Cancel) 
{
	static CMyCriticalSection cs;
	cs.enter();

	CIEDlg* dlg = new CIEDlg( NULL, NULL );
	dlg->Create( LPCTSTR(IDD_IEFORM), GetDesktopWindow() );
	dlg->setPluginInstance( this->m_PluginInst );
	CIETab::pushToPool( dlg );
	*ppDisp = dlg->getWebBrowser()->get_Application();

	CIETabScriptable* pObj = m_PluginInst->getScriptableObject();
	if( pObj )
	{
		pObj->requestNewTab( L"" );
	}

	cs.leave();
}

class CFocusWnd : public CWnd
{
public:
	CFocusWnd() {};

	afx_msg void OnSetFocus(CWnd *pWnd);
	DECLARE_MESSAGE_MAP()

};

BEGIN_MESSAGE_MAP(CFocusWnd, CWnd)
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

void CFocusWnd::OnSetFocus(CWnd *pWnd) 
{
	ATLTRACE("got a focus");
}

// Work around an MSDN definition bug
#undef SubclassWindow 

void AttachAll(HWND hWnd);

CWebBrowserCtrl* WebBrowserFromHandle( HWND hwnd );

static TCHAR gszSubclassProp[] = _T("__ietab_subclassid");
static UINT guIdSubclass = 39421;

LRESULT CALLBACK SubclassProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
)
{
	switch(uMsg)
	{
	case WM_NCDESTROY:
		{
			UINT uId = (UINT) GetProp(hWnd, gszSubclassProp);
			if(uId != 0)
			{
				RemoveProp(hWnd, gszSubclassProp);
				RemoveWindowSubclass(hWnd, SubclassProc, guIdSubclass);
			}
		}
		break;

	case WM_PARENTNOTIFY:
		if(LOWORD(wParam) == WM_CREATE)
		{
			bool bSubclass = true;

			// We stop at "Internet Explorer_Server
			TCHAR szClassName[MAX_PATH];
			if ( GetClassName( hWnd, szClassName, ARRAYSIZE(szClassName) ) > 0 )
			{
				if ( _tcscmp(szClassName, _T("Internet Explorer_Server")) == 0 )
				{
					bSubclass = false;
				}
			}

			if(bSubclass)
			{
				HWND hWndChild = (HWND) lParam;
				UINT uId = (UINT) GetProp(hWndChild, gszSubclassProp);
				ATLASSERT(uId == 0);
				if(uId == 0)
				{
					AttachAll(hWndChild);
				}
			}
		}
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		{
			CWebBrowserCtrl* wb = WebBrowserFromHandle(hWnd);
			if(wb)
			{
				if( (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) &&
					( HIBYTE(GetKeyState(VK_CONTROL)) || // Ctrl is pressed
					  HIBYTE(GetKeyState(VK_MENU)) || // Alt is pressed
					  ((wParam >= VK_F1) && (wParam <= VK_F24)) // Function-Keys is pressed
					)
				  )
				{
					// Redirect to browser
					::PostMessage( ::GetParent(::GetParent(::GetParent(::GetParent(wb->m_hWnd)))), uMsg, wParam, lParam );
				}
			}
		}
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


void AttachAll(HWND hWnd)
{
	UINT uId = (UINT) GetProp(hWnd, gszSubclassProp);
	ATLASSERT(uId == 0);
	if(uId == 0)
	{
		SetProp(hWnd, gszSubclassProp, (HANDLE) guIdSubclass);
		SetWindowSubclass(hWnd, SubclassProc, guIdSubclass, NULL);
	}

	// Subclass all of the children
	hWnd = ::GetWindow(hWnd, GW_CHILD);
	while(hWnd) {
		AttachAll(hWnd);
		hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
	}
}

BOOL CIEDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_WB.Create( NULL, NULL, WS_CHILD|WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_IE );

	m_WB.put_RegisterAsBrowser( TRUE );
	m_WB.put_RegisterAsDropTarget( TRUE );

	// We want to subclass all the child windows so we can detect focus changes.
	// This will enable us to notify FF of a change in element focus because it gets confused.
	AttachAll(m_hWnd);

	if( m_URL && *m_URL )	{
		COleVariant url(m_URL);
		m_WB.Navigate2( &url, NULL, NULL, NULL, NULL );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CIEDlg::OnEraseBkgnd(CDC* pDC) 
{
	CRect rc;
	GetClientRect( rc );
	pDC->FillSolidRect( rc, GetSysColor(COLOR_WINDOW) );
	return TRUE;
}

void CIEDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

LRESULT CIEDlg::OnUpdateTitle(WPARAM wp, LPARAM lp)
{
	if (m_PluginInst) {
		const char* scriptStr = "javascript:document.title=document.getElementById(\'IETab2\').title;";
		NPN_GetURL( m_PluginInst->getInstance(), scriptStr, NULL );
	}
	return LRESULT(0);
}

LRESULT CIEDlg::OnUpdateStatus(WPARAM wp, LPARAM lp)
{
	if (m_PluginInst) {
		UINT len = m_StatusText.GetLength() + 1;
		UINT ulen = MultiByteToWideChar( CP_ACP, 0, LPCTSTR(m_StatusText), len, NULL, 0 );
		WCHAR* utext = new WCHAR[ ulen ];
		if( utext )	{
			ulen = MultiByteToWideChar( CP_ACP, 0, LPCTSTR(m_StatusText), len, utext, ulen );
			len = WideCharToMultiByte( CP_UTF8, 0, utext, ulen, NULL, 0, NULL, NULL );
			char* u8text = new char[ len ];
			WideCharToMultiByte( CP_UTF8, 0, utext, ulen, u8text, len, NULL, NULL );
			NPN_Status( m_PluginInst->getInstance(), u8text );
			delete []u8text;
			delete []utext;
			return 0;
		}
		else NPN_Status( m_PluginInst->getInstance(), "" );
	}
	return LRESULT(0);
}

void CIEDlg::OnTitleChange(LPCTSTR Text)
{
	PostMessage( WM_IE_UPDATE_TITLE );
}

void CIEDlg::OnCommandStateChange(long Command, BOOL Enable)
{
	switch( Command )
	{
	case CSC_NAVIGATEBACK:
		m_WB.enableBack( Enable?true:false );
		break;
	case CSC_NAVIGATEFORWARD:
		m_WB.enableForward( Enable?true:false );
		break;
	}
}

void CIEDlg::OnStatusChange(LPCTSTR Text)
{
	m_StatusText = Text;
	PostMessage( WM_IE_UPDATE_STATUS );
}

void CIEDlg::OnBeforeNavigate2(LPDISPATCH pDisp, VARIANT FAR* URL, VARIANT FAR* Flags, VARIANT FAR* TargetFrameName, VARIANT FAR* PostData, VARIANT FAR* Headers, BOOL FAR* Cancel) 
{
}

void CIEDlg::OnDownloadBegin()
{
}

void CIEDlg::OnProgressChange(long Progress, long ProgressMax)
{
	if (!m_PluginInst) return;

	CIETabScriptable* pObj = m_PluginInst->getScriptableObject();;
	if (pObj)
	{
		if (Progress == -1) Progress = ProgressMax;
		if (ProgressMax) Progress = (100*Progress)/ProgressMax; else Progress = -1;
		pObj->m_lProgress = Progress;
		pObj->requestProgressChange(Progress);
	}
}

void CIEDlg::OnDocumentCompleteIe(LPDISPATCH pDisp, VARIANT* URL)
{
	if (!m_PluginInst) return;

	const char* scriptStr = "javascript:document.title=document.getElementById(\'IETab2\').title;";
	NPN_GetURL( m_PluginInst->getInstance(), scriptStr, NULL );

	CIETabScriptable* pObj = m_PluginInst->getScriptableObject();;
	if (pObj)
	{
		pObj->m_lProgress = -1;
		pObj->requestProgressChange(-1);
	}
}

void CIEDlg::OnSetSecureLockIcon(long SecureLockIcon)
{
	if (!m_PluginInst) return;

	CIETabScriptable* pObj = m_PluginInst->getScriptableObject();;
	if (pObj)
	{
		pObj->m_lSecurity = SecureLockIcon;
		pObj->requestSecurityChange(SecureLockIcon);
	}
}

void CIEDlg::OnWindowClosing(BOOL IsChildWindow, BOOL* Cancel)
{
	if (!m_PluginInst) return;

	CIETabScriptable* pObj = m_PluginInst->getScriptableObject();
	if(pObj)
	{
		pObj->requestCloseWindow();
	}

	*Cancel = TRUE;
}

void CIEDlg::OnTimer(UINT_PTR nIDEvent)
{
	stopAutoDestroy();
	DestroyWindow();
}


// Custom control site

class CIETabControlSite : public COleControlSite
{
public:
	CIETabControlSite(COleControlContainer* pContainer, CIEDlg* dlg);
	~CIETabControlSite();

	BEGIN_INTERFACE_PART(DocHostUIHandler, IDocHostUIHandler)
		STDMETHOD(ShowContextMenu)(DWORD, LPPOINT, LPUNKNOWN, LPDISPATCH);
		STDMETHOD(GetHostInfo)(DOCHOSTUIINFO*);
		STDMETHOD(ShowUI)(DWORD, LPOLEINPLACEACTIVEOBJECT,
			LPOLECOMMANDTARGET, LPOLEINPLACEFRAME, LPOLEINPLACEUIWINDOW);
		STDMETHOD(HideUI)(void);
		STDMETHOD(UpdateUI)(void);
		STDMETHOD(EnableModeless)(BOOL);
		STDMETHOD(OnDocWindowActivate)(BOOL);
		STDMETHOD(OnFrameWindowActivate)(BOOL);
		STDMETHOD(ResizeBorder)(LPCRECT, LPOLEINPLACEUIWINDOW, BOOL);
		STDMETHOD(TranslateAccelerator)(LPMSG, const GUID*, DWORD);
		STDMETHOD(GetOptionKeyPath)(OLECHAR **, DWORD);
		STDMETHOD(GetDropTarget)(LPDROPTARGET, LPDROPTARGET*);
		STDMETHOD(GetExternal)(LPDISPATCH*);
		STDMETHOD(TranslateUrl)(DWORD, OLECHAR*, OLECHAR **);
		STDMETHOD(FilterDataObject)(LPDATAOBJECT , LPDATAOBJECT*);
	END_INTERFACE_PART(DocHostUIHandler)

	BEGIN_INTERFACE_PART(OleCommandTarget, IOleCommandTarget)
		STDMETHOD(Exec)( 
			/* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
			/* [in] */ DWORD nCmdID,
			/* [in] */ DWORD nCmdexecopt,
			/* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
			/* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut);

		STDMETHOD(QueryStatus)(
			/* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
			/* [in] */ ULONG cCmds,
			/* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
			/* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText);
	END_INTERFACE_PART(OleCommandTarget)

	DECLARE_INTERFACE_MAP()

protected:
	CIEDlg* m_dlg;
};

BEGIN_INTERFACE_MAP(CIETabControlSite, COleControlSite)
	INTERFACE_PART(CIETabControlSite, IID_IOleCommandTarget, OleCommandTarget)
	INTERFACE_PART(CIETabControlSite, IID_IDocHostUIHandler, DocHostUIHandler)
END_INTERFACE_MAP()

CIETabControlSite::CIETabControlSite(COleControlContainer* pContainer, CIEDlg* dlg)
: COleControlSite(pContainer), m_dlg(dlg)
{
}

CIETabControlSite::~CIETabControlSite()
{
}


ULONG FAR EXPORT CIETabControlSite::XOleCommandTarget::AddRef()
{
	METHOD_PROLOGUE_(CIETabControlSite, OleCommandTarget)
	return pThis->ExternalAddRef();
}


ULONG FAR EXPORT CIETabControlSite::XOleCommandTarget::Release()
{                            
    METHOD_PROLOGUE_(CIETabControlSite, OleCommandTarget)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CIETabControlSite::XOleCommandTarget::QueryInterface(REFIID riid, void **ppvObj)
{
	METHOD_PROLOGUE_(CIETabControlSite, OleCommandTarget)
    HRESULT hr = (HRESULT)pThis->ExternalQueryInterface(&riid, ppvObj);
	return hr;
}


STDMETHODIMP CIETabControlSite::XOleCommandTarget::Exec(
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [in] */ DWORD nCmdexecopt,
            /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut
		   )
{
	HRESULT hr = S_OK;
	BOOL bActiveX = FALSE;
	if ( pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CGID_DocHostCommandHandler))
	{
		// Disable script error
		if((nCmdID == 40 || nCmdID == 41) )	//OLECMDID_SHOWSCRIPTERROR
		{
			if(nCmdID == 41)
			{
				IHTMLDocument2*             pDoc = NULL;
				IHTMLWindow2*               pWindow = NULL;
				IHTMLEventObj*              pEventObj = NULL;
				BSTR                        rgwszName = SysAllocString(L"messageText");
				DISPID                      rgDispID;
				VARIANT                     rgvaEventInfo;
				DISPPARAMS                  params;

				params.cArgs = 0;
				params.cNamedArgs = 0;

				// Get the document that is currently being viewed.
				hr = pvaIn->punkVal->QueryInterface(IID_IHTMLDocument2, (void **) &pDoc);				
				// Get document.parentWindow.
				hr = pDoc->get_parentWindow(&pWindow);
				pDoc->Release();
				// Get the window.event object.
				hr = pWindow->get_event(&pEventObj);
				// Get the error info from the window.event object.
				// Get the property's dispID.
				hr = pEventObj->GetIDsOfNames(IID_NULL, &rgwszName, 1, 
							LOCALE_SYSTEM_DEFAULT, &rgDispID);
				// Get the value of the property.
				hr = pEventObj->Invoke(rgDispID, IID_NULL,LOCALE_SYSTEM_DEFAULT,
							DISPATCH_PROPERTYGET, &params, &rgvaEventInfo,NULL, NULL);
				SysFreeString(rgwszName);
				pWindow->Release();
				pEventObj->Release();
				if(rgvaEventInfo.byref && wcsstr(rgvaEventInfo.bstrVal, L" ActiveX ")!=NULL)
					bActiveX = TRUE;
				SysFreeString(rgvaEventInfo.bstrVal);
			}

			(*pvaOut).vt = VT_BOOL;
			// Continue running scripts on the page.
			(*pvaOut).boolVal = VARIANT_TRUE;
		
			if(nCmdID == 40)
				;//pMainFrame->SetMessageText("Script Error!");
			else if(bActiveX)
				;//pMainFrame->SetMessageText("ActiveX Denied!");
			else
			{
				(*pvaOut).boolVal = VARIANT_FALSE;
				return OLECMDERR_E_NOTSUPPORTED;
			}
			return hr;
		}
	}
	return OLECMDERR_E_NOTSUPPORTED;
}

STDMETHODIMP CIETabControlSite::XOleCommandTarget::QueryStatus(
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ ULONG cCmds,
            /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
            /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText
		   )
{
	METHOD_PROLOGUE_(CIETabControlSite, OleCommandTarget)
    return OLECMDERR_E_NOTSUPPORTED;
}


STDMETHODIMP CIETabControlSite::XDocHostUIHandler::GetExternal(LPDISPATCH *lppDispatch)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_FALSE;
}


#define IDR_BROWSE_CONTEXT_MENU  24641
#define IDR_FORM_CONTEXT_MENU    24640
#define SHDVID_GETMIMECSETMENU   27
#define SHDVID_ADDMENUEXTENSIONS 53
#define IDM_OPEN_IN_FIREFOX      (IDM_MENUEXT_LAST__ + 1)

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::ShowContextMenu(
	DWORD dwID, LPPOINT ppt, LPUNKNOWN pcmdTarget, LPDISPATCH pdispReserved)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)

    HRESULT hr;
    HINSTANCE hinstSHDOCLC;
    HWND hwnd;
    HMENU hMenu;
    CComPtr<IOleCommandTarget> spCT;
    CComPtr<IOleWindow> spWnd;
    MENUITEMINFO mii = {0};
    CComVariant var, var1, var2;

    hr = pcmdTarget->QueryInterface(IID_IOleCommandTarget, (void**)&spCT);
    hr = pcmdTarget->QueryInterface(IID_IOleWindow, (void**)&spWnd);
    hr = spWnd->GetWindow(&hwnd);

    hinstSHDOCLC = LoadLibrary(TEXT("SHDOCLC.DLL"));
    
    if (hinstSHDOCLC == NULL)
    {
        // Error loading module -- fail as securely as possible
        return S_FALSE;
    }

    hMenu = LoadMenu(hinstSHDOCLC,
                     MAKEINTRESOURCE(IDR_BROWSE_CONTEXT_MENU));

    hMenu = GetSubMenu(hMenu, dwID);

    // Get the language submenu
    hr = spCT->Exec(&CGID_ShellDocView, SHDVID_GETMIMECSETMENU, 0, NULL, &var);

    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_SUBMENU;
    mii.hSubMenu = (HMENU) var.byref;

    // Add language submenu to Encoding context item
    SetMenuItemInfo(hMenu, IDM_LANGUAGE, FALSE, &mii);

    // Insert Shortcut Menu Extensions from registry
    V_VT(&var1) = VT_INT_PTR;
    V_BYREF(&var1) = hMenu;

    V_VT(&var2) = VT_I4;
    V_I4(&var2) = dwID;

    hr = spCT->Exec(&CGID_ShellDocView, SHDVID_ADDMENUEXTENSIONS, 0, &var1, &var2);

	if( dwID == CONTEXT_MENU_ANCHOR ) {
		// Add Open in Firefox menu
//		InsertMenu( hMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0 );
//		InsertMenu( hMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, IDM_OPEN_IN_FIREFOX, "Open in Firefox Tab" );
	}

    // Show shortcut menu
    int iSelection = ::TrackPopupMenu(hMenu,
                                      TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                      ppt->x,
                                      ppt->y,
                                      0,
                                      hwnd,
                                      (RECT*)NULL);

	if( iSelection == IDM_OPEN_IN_FIREFOX ) {
		// Open in Firefox
		CIETabScriptable *pObj;
		pObj = pThis->m_dlg->getScriptableObject();
		if( pObj )	{
			pObj->requestNewTab( L"about:blank" );
		}
	}
	else {
		// Send selected shortcut menu item command to shell
		LRESULT lr = ::SendMessage(hwnd, WM_COMMAND, iSelection, NULL);
	}
    FreeLibrary(hinstSHDOCLC);
    return S_OK;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::GetHostInfo(
	DOCHOSTUIINFO *pInfo)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)

	pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER
					|DOCHOSTUIFLAG_THEME
					|DOCHOSTUIFLAG_ENABLE_FORMS_AUTOCOMPLETE
					|DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK
					|DOCHOSTUIFLAG_CODEPAGELINKEDFONTS;
	pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

	return S_OK;
}


STDMETHODIMP CIETabControlSite::XDocHostUIHandler::ShowUI(
	DWORD dwID, LPOLEINPLACEACTIVEOBJECT pActiveObject,
	LPOLECOMMANDTARGET pCommandTarget, LPOLEINPLACEFRAME pFrame,
	LPOLEINPLACEUIWINDOW pDoc)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_FALSE;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::HideUI(void)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_OK;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::UpdateUI(void)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_OK;
}


STDMETHODIMP CIETabControlSite::XDocHostUIHandler::EnableModeless(BOOL fEnable)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_OK;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::OnDocWindowActivate(BOOL fActivate)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_OK;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::OnFrameWindowActivate(
	BOOL fActivate)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_OK;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::ResizeBorder(
	LPCRECT prcBorder, LPOLEINPLACEUIWINDOW pUIWindow, BOOL fFrameWindow)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_OK;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::TranslateAccelerator(
	LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_FALSE;
}


STDMETHODIMP CIETabControlSite::XDocHostUIHandler::GetOptionKeyPath(
	LPOLESTR* pchKey, DWORD dwReserved)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_FALSE;
}


STDMETHODIMP CIETabControlSite::XDocHostUIHandler::GetDropTarget(
	LPDROPTARGET pDropTarget, LPDROPTARGET* ppDropTarget)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_FALSE;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::TranslateUrl(
	DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_FALSE;
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::FilterDataObject(
	LPDATAOBJECT pDataObject, LPDATAOBJECT* ppDataObject)
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return S_FALSE;
}


STDMETHODIMP_(ULONG) CIETabControlSite::XDocHostUIHandler::AddRef()
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CIETabControlSite::XDocHostUIHandler::Release()
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return pThis->ExternalRelease();
}

STDMETHODIMP CIETabControlSite::XDocHostUIHandler::QueryInterface(
		  REFIID iid, LPVOID far* ppvObj)     
{
	METHOD_PROLOGUE_EX_(CIETabControlSite, DocHostUIHandler)
	return pThis->ExternalQueryInterface(&iid, ppvObj);
}


BOOL CIEDlg::CreateControlSite(COleControlContainer* pContainer, 
   COleControlSite** ppSite, UINT /* nID */, REFCLSID /* clsid */)
{
	ASSERT(ppSite != NULL);
	*ppSite = new CIETabControlSite(pContainer, this);
	return TRUE;
}

void CIEDlg::OnDestroy()
{
	m_WB.DestroyWindow();
}

BOOL RegReadStr( HKEY key, const char* name, char* buf, DWORD len ) {
	DWORD type = REG_SZ;
	if( ERROR_SUCCESS == RegQueryValueExA( key, name, 0, &type, (LPBYTE)buf, &len ) )
		return TRUE;
	return FALSE;
}

BOOL IsRegKeyYes( HKEY key, const char* name ){
	char val[10];
	if( RegReadStr(key, name, val, sizeof(val) )
		&& 0 == strcmp(val, "yes") ) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsRegKeyNo( HKEY key, const char* name ){
	char val[10];
	if( RegReadStr(key, name, val, sizeof(val) )
		&& 0 == strcmp(val, "no") ) {
		return TRUE;
	}
	return FALSE;
}

/*
BOOL CIEDlg::OnAmbientProperty(COleControlSite* pSite, DISPID dispid, VARIANT* pvar)
{
	if (dispid == DISPID_AMBIENT_DLCONTROL)
	{
		DWORD prop = DLCTL_DLIMAGES|DLCTL_VIDEOS|DLCTL_BGSOUNDS;
		HKEY reg = NULL;
		if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER,
									_T("Software\\Microsoft\\Internet Explorer\\Main"),
									0, KEY_QUERY_VALUE, &reg ) )
		{
			char val[32];
			if( IsRegKeyNo(reg, "Display Inline Images" ) ) {
				prop &= ~DLCTL_DLIMAGES;
			}
			if( IsRegKeyNo(reg, "Display Inline Videos" ) ) {
				prop &= ~DLCTL_VIDEOS;
			}
			if( IsRegKeyNo(reg, "Play_Background_Sounds" ) ) {
				prop &= ~DLCTL_BGSOUNDS;
			}
			if( IsRegKeyNo(reg, "Play_Animations" ) ) {
				prop &= ~DLCTL_BGSOUNDS;
			}

			RegCloseKey( reg );
			pvar->vt = VT_I4;
			pvar->lVal = prop;
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
	return CDialog::OnAmbientProperty(pSite, dispid, pvar);
}
*/

void CIEDlg::OnWindowSetResizable(BOOL Resizable)
{
	resizable = (Resizable?true:false);
	if( resizable ) {
		CRect rc;
		GetClientRect(rc);
		m_WB.SetWindowPos( NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOZORDER );
		width = 0;
		height = 0;
	}
}

void CIEDlg::OnWindowSetWidth(long Width)
{
	CRect rc;
	m_WB.GetWindowRect(rc);
	width = Width;
	m_WB.SetWindowPos( NULL, 0, 0, Width, rc.Height(), SWP_NOZORDER );
}

void CIEDlg::OnWindowSetHeight(long Height)
{
	CRect rc;
	m_WB.GetWindowRect(rc);
	height = Height;
	m_WB.SetWindowPos( NULL, 0, 0, rc.Width(), Height, SWP_NOZORDER );
}
