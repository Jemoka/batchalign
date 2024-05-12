// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__EB2FA987_D6AD_11D0_8D74_00A02481E866__INCLUDED_)
#define AFX_STDAFX_H__EB2FA987_D6AD_11D0_8D74_00A02481E866__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#define _AFX_NO_BSTR_SUPPORT
// #include <afxwin.h>         // MFC core and standard components
//#include <afxext.h>         // MFC extensions
//#include <afxdisp.h>        // MFC OLE automation classes
#include <afxres.h>
//#ifndef _AFX_NO_AFXCMN_SUPPORT
//#include <afxcmn.h>			// MFC support for Windows Common Controls
//#endif // _AFX_NO_AFXCMN_SUPPORT

//#include <atlbase.h>
//#include <atlcom.h>
//#include <atlhost.h>
//#include <atlctl.h>

//#include <wmp.h>

struct RGBColor {
	unsigned short      red;                    /*magnitude of red component*/
	unsigned short      green;                  /*magnitude of green component*/
	unsigned short      blue;                   /*magnitude of blue component*/
};

typedef unsigned char		UInt8;
typedef unsigned short		UInt16;
typedef unsigned long		UInt32;
typedef signed long			SInt32;
typedef unsigned char		UInt8;
typedef boolean				Boolean;


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__EB2FA987_D6AD_11D0_8D74_00A02481E866__INCLUDED_)
