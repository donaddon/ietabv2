/////////////////////////////////////////////////////////////////////////////
// Name:        ietab.cpp
// Purpose:     Display an IE-based window
// Author:      PCMan (HZY)   http://pcman.sayya.org/
// Maintainer:  IETAB.NET http://www.ietab.net
// E-mail:      support@ietab.net
// Created:     2004.7.17
// Copyright:   (C) 2004 PCMan
// License:     GPL : http://www.gnu.org/licenses/gpl.html
//
// Modified by: 
//   2/24/10 - 3/8/10 -- ietab.net
//       Converted to npruntime
//       Updated Purpose description to be correct.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
  #pragma implementation "ietab.h"
#endif

#include "ietab.h" // class's header file
#include "resource.h"
#include "plugin.h"
#include <commctrl.h>
#include ".\ietab.h"
#include "ScriptableObject.h"

#define	WM_INIT		WM_APP+100

CList<CIEDlg*> CIETab::browserPool;

LRESULT CALLBACK CIETab::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	nsPluginInstance* pi = (nsPluginInstance*)GetWindowLong( hwnd, GWL_USERDATA );
	if( ! pi )
		return 0;
	CIETab* pView = (CIETab*)pi->m_pIETab;
	return pView->WndProc( msg, wp, lp );
}

// class constructor
CIETab::CIETab( HWND hWnd, const char* url )
	: m_hWnd(hWnd), m_pIEWnd( NULL ), m_URL( url )
{
}

// class destructor
CIETab::~CIETab()
{
	if( m_pIEWnd )
	{
		m_pIEWnd->DestroyWindow();
		m_pIEWnd = NULL;
	}

	m_hWnd = NULL;
}

void CIETab::OnSize( int SizeType, int cx, int cy )
{
	if( m_pIEWnd && m_pIEWnd->m_hWnd )
		m_pIEWnd->SetWindowPos( NULL, 0, 0, cx, cy, SWP_NOMOVE|SWP_NOZORDER );
}

#ifdef	_USRDLL
	#define DLLAPI	__declspec(dllexport)
#else
	#define DLLAPI	__declspec(dllimport)
#endif


LRESULT CIETab::WndProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch( msg )
	{
	case WM_SIZE:
		OnSize( wp, LOWORD(lp), HIWORD(lp) );
		return 0;
	}
	return CallWindowProc( m_OldWndProc, m_hWnd, msg, wp, lp );
}


void CIETab::init()
{
	nsPluginInstance* pi = (nsPluginInstance*)GetWindowLong( m_hWnd, GWL_USERDATA );
	AfxEnableControlContainer();
	CWnd parent;
	parent.Attach( m_hWnd );
	parent.ModifyStyle( 0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS );

	m_pIEWnd = CIETab::popFromPool();

	if( m_pIEWnd )	{	// this windows is from the pool
		m_pIEWnd->stopAutoDestroy();
		m_pIEWnd->SetParent( &parent );
		m_pIEWnd->setPluginInstance( pi );
	}
	else{
		m_pIEWnd = new CIEDlg( &parent, pi );
		m_pIEWnd->m_URL = m_URL;
		if( ! m_pIEWnd->Create( LPCTSTR(IDD_IEFORM), &parent ) ) {
			delete m_pIEWnd;
			m_pIEWnd = NULL;
		}

	}

	parent.Detach();

	if(m_pIEWnd)
	{
		m_pIEWnd->ShowWindow(SW_SHOW);

//		nsScriptablePeer* sp = pi->getScriptablePeer();

//		sp->setBrowser( m_pIEWnd->getWebBrowser() );
//		NS_RELEASE(sp);

		CIETabScriptable *pScriptable = pi->getScriptableObject();

		pScriptable->setBrowser( m_pIEWnd->getWebBrowser() );

		CRect rc;
		::GetWindowRect(m_hWnd, rc);
		OnSize( 0, rc.Width(), rc.Height() );

		m_OldWndProc = (WNDPROC)SetWindowLongPtr( m_hWnd, GWL_WNDPROC, (LONG_PTR)(WNDPROC)CIETab::WndProc);
	}

}


void CIETab::destroy(void)
{
	SetWindowLong( m_hWnd, GWL_WNDPROC, (long)m_OldWndProc);
	SetWindowLong( m_hWnd, GWL_USERDATA, 0 );
}

void CIETab::cleanupPool(void)
{
	POSITION pos = browserPool.GetHeadPosition();
	for(; pos; browserPool.GetNext(pos) )	{
		CIEDlg* dlg = browserPool.GetAt( pos );
		dlg->stopAutoDestroy();
		dlg->DestroyWindow();
		//	delete dlg is not needed
		//  it is deleted in CIEDlg::PostNcDestroy()
	}
	browserPool.RemoveAll();
}

CIEDlg* CIETab::popFromPool(void)
{
	POSITION pos = browserPool.GetHeadPosition();
	if( pos ){
		CIEDlg* dlg = browserPool.GetAt( pos );
		browserPool.RemoveAt(pos);
		return dlg;
	}
	return NULL;
}


void CIETab::pushToPool(CIEDlg* dlg)
{
	dlg->ShowWindow( SW_HIDE );

	// Detach from its original parent window
	::SetParent( dlg->m_hWnd, ::GetDesktopWindow() );

	// Push the browser window into pool
	browserPool.AddTail( dlg );
	dlg->startAutoDestroy();
}

