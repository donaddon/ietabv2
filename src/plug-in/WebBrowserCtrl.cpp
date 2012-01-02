#include ".\webbrowserctrl.h"
#include <oleidl.h>
#include "resource.h"
#include <afxcoll.h>

#include <windowsx.h>

#include <exdispid.h>
#include <mshtmcid.h>
#include <mshtmhst.h>
#include <mshtml.h>
#include <afxocc.h>

CMapPtrToPtr IEMap;
static HHOOK IEHook = NULL;

#define IS_CHILD(hwnd)	(GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)

CWebBrowserCtrl* WebBrowserFromHandle( HWND hwnd )
{
	CWebBrowserCtrl* wb;
	for( ; hwnd && IS_CHILD(hwnd) ; hwnd = GetParent(hwnd) )
	{
		if( IEMap.Lookup( (void*)hwnd, (void*&)wb ) )
			return wb;
	}
	return NULL;
}

static LRESULT GetMsgHook(int code, WPARAM wp, LPARAM lp)
{
	if( code < 0 || wp != PM_REMOVE ) return CallNextHookEx( IEHook, code, wp, lp );

	MSG* lpMsg = (MSG*)lp;
	CWebBrowserCtrl* wb = WebBrowserFromHandle(lpMsg->hwnd);
	if( wb )
	{
		/* Only process the messages we want to minimize overhead */
		if( lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST )
		{
			IDispatch* app = wb->get_Application();
			if( app )	{
				IOleInPlaceActiveObject* ip = NULL;
				if( SUCCEEDED( app->QueryInterface( IID_IOleInPlaceActiveObject, (void**)&ip ) ) )	{
					if( S_OK == ip->TranslateAccelerator( lpMsg ) )
					{
						lpMsg->message = 0;
					}
					ip->Release();
				}
				app->Release();
			}
/*

THIS NO LONGER WORKS WHEN WE RUN OUT-OF-PROCESS (Fx's new oop model).  So instead of using a message hook
we subclass every child window with AttachAll, and forward the control keys using that subclass procedure
			if( (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_SYSKEYDOWN) &&
			    ( HIBYTE(GetKeyState(VK_CONTROL)) || // Ctrl is pressed
			      HIBYTE(GetKeyState(VK_MENU)) || // Alt is pressed
			      ((lpMsg->wParam >= VK_F1) && (lpMsg->wParam <= VK_F24)) // Function-Keys is pressed
			    )
			  )
			{
				// Redirect to browser

				// If re-enabled, consider using ::SendMessage
				::SendMessage( ::GetParent(::GetParent(::GetParent(::GetParent(wb->m_hWnd)))), lpMsg->message, lpMsg->wParam, lpMsg->lParam );
			}
*/
		}
	}
	return CallNextHookEx( IEHook, code, wp, lp );
}


CWebBrowserCtrl::CWebBrowserCtrl(void)
: canBack(false)
, canForward(false)
{
}

CWebBrowserCtrl::~CWebBrowserCtrl(void)
{

}

bool CWebBrowserCtrl::GoBack(void)
{
	if( !canBack )
		return false;
	LPDISPATCH lpd = get_Application();
	IWebBrowser2* pwb=NULL;
	if( SUCCEEDED(lpd->QueryInterface(IID_IWebBrowser2,(void**)&pwb)) )
	{
		HRESULT res = pwb->GoBack();
		pwb->Release();
		if( res == S_OK )
			return true;
	}
	lpd->Release();
	return false;
}

bool CWebBrowserCtrl::GoForward(void)
{
	if( !canForward )
		return false;
	LPDISPATCH lpd = get_Application();
	IWebBrowser2* pwb=NULL;
	if( SUCCEEDED(lpd->QueryInterface(IID_IWebBrowser2,(void**)&pwb)) )
	{
		HRESULT res = pwb->GoForward();
		pwb->Release();
		if( res == S_OK )
			return true;
	}
	lpd->Release();
	return false;
}

/*
const DWORD SET_FEATURE_ON_THREAD = 0x00000001;
const DWORD SET_FEATURE_ON_PROCESS = 0x00000002;
const DWORD SET_FEATURE_IN_REGISTRY = 0x00000004;
const DWORD SET_FEATURE_ON_THREAD_LOCALMACHINE = 0x00000008;
const DWORD SET_FEATURE_ON_THREAD_INTRANET = 0x00000010;
const DWORD SET_FEATURE_ON_THREAD_TRUSTED = 0x00000020;
const DWORD SET_FEATURE_ON_THREAD_INTERNET = 0x00000040;
const DWORD SET_FEATURE_ON_THREAD_RESTRICTED = 0x00000080;

enum INTERNETFEATURELIST {
	FEATURE_OBJECT_CACHING = 0,
	FEATURE_ZONE_ELEVATION = 1,
	FEATURE_MIME_HANDLING = 2,
	FEATURE_MIME_SNIFFING = 3,
	FEATURE_WINDOW_RESTRICTIONS = 4,
	FEATURE_WEBOC_POPUPMANAGEMENT = 5,
	FEATURE_BEHAVIORS = 6,
	FEATURE_DISABLE_MK_PROTOCOL = 7,
	FEATURE_LOCALMACHINE_LOCKDOWN = 8,
	FEATURE_SECURITYBAND = 9,
	FEATURE_RESTRICT_ACTIVEXINSTALL = 10,
	FEATURE_VALIDATE_NAVIGATE_URL = 11,
	FEATURE_RESTRICT_FILEDOWNLOAD = 12,
	FEATURE_ADDON_MANAGEMENT = 13,
	FEATURE_PROTOCOL_LOCKDOWN = 14,
	FEATURE_HTTP_USERNAME_PASSWORD_DISABLE = 15,
	FEATURE_SAFE_BINDTOOBJECT = 16,
	FEATURE_UNC_SAVEDFILECHECK = 17,
	FEATURE_GET_URL_DOM_FILEPATH_UNENCODED = 18,
	FEATURE_ENTRY_COUNT = 19,
};*/

typedef HRESULT (WINAPI *PCoInternetSetFeatureEnabled)(INTERNETFEATURELIST, DWORD, BOOL);

BOOL CWebBrowserCtrl::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	BOOL ret = CWebBrowser2::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	if( ret )
	{
		IEMap.SetAt( (void*)m_hWnd, this );
		if( ! IEHook ){	// First control
			IEHook = SetWindowsHookEx( WH_GETMESSAGE, (HOOKPROC)GetMsgHook, NULL, GetCurrentThreadId() );
			HMODULE urlmon = LoadLibrary("urlmon.dll");
			if( urlmon ){
				PCoInternetSetFeatureEnabled CoInternetSetFeatureEnabled = NULL;
				CoInternetSetFeatureEnabled = (PCoInternetSetFeatureEnabled)GetProcAddress( urlmon, "CoInternetSetFeatureEnabled" );;
				if( CoInternetSetFeatureEnabled ) {
					CoInternetSetFeatureEnabled( FEATURE_WEBOC_POPUPMANAGEMENT, SET_FEATURE_ON_PROCESS, TRUE );
					CoInternetSetFeatureEnabled( FEATURE_SECURITYBAND, SET_FEATURE_ON_PROCESS, TRUE );
				}
				FreeLibrary( urlmon );
			}
		}
	}
	return ret;
}

BOOL CWebBrowserCtrl::DestroyWindow()
{
	if( m_hWnd ) {
		IEMap.RemoveKey( (void*)m_hWnd );
	}

	if( IEMap.GetCount() == 0 && IEHook )	{
		UnhookWindowsHookEx( IEHook );
		IEHook = NULL;
	}
	return CWebBrowser2::DestroyWindow();
}
