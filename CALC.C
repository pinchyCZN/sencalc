//
//    Calculator
//
//   3-6 Aug 2001 - Win32 window version
//    10 Aug 2001 - fix: lost parm in Dlg_Proc (crash in win2000)
//                - font size manipulation  
//

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdarg.h>
#include "resource.h"

typedef  unsigned char     BYTE;
typedef  unsigned short    WORD;
typedef  unsigned long     DWORD;

#include "sencalc.h"

/// defines ////////////////////////////////////////////////

#define  BUFFER_LENMAX     128

enum{
   TITLE_NONE=0,
   TITLE_SIGNED,
   TITLE_UNSIGNED,
   TITLE_BINARY,
   TITLE_HEX,
   TITLE_STRING,
   };

/// prototypes /////////////////////////////////////////////

void ShowTitle(HWND);
BYTE
 *CharsShow( BYTE *a, int n ),
 *ErrorLine( int errPos, BYTE *msgErr ),
 *FString( BYTE *format, ... ),
 *ToBinary( DWORD result ),
 *ToBinary64( unsigned __int64 result );
int
 ID2Index( UINT id ),
 Printable( BYTE ch );
BOOL WINAPI
  Dlg_Proc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

/// global variables ///////////////////////////////////////

BYTE
 *titleFmt[]={ "", "%ld", "%lu", "%s", "%.8lX", "\"%s\"" },
 temp[ BUFFER_LENMAX ],
 temp2[ BUFFER_LENMAX ];
int
 iTitle=TITLE_HEX;

/// functions //////////////////////////////////////////////

int WINAPI
 WinMain( HINSTANCE hinst, HINSTANCE hprev, PSTR pszCmdLine, int nCmdShow )
{
	HACCEL haccel;
	HWND hdlg;
	MSG msg;
	haccel=LoadAccelerators(hinst,MAKEINTRESOURCE(IDR_ACCELERATOR1));
	hdlg=CreateDialog(hinst,MAKEINTRESOURCE(IDD_SENCALC),NULL,Dlg_Proc);
	while(GetMessage(&msg,NULL,0,0)){
		if(haccel!=0){
			if(TranslateAccelerator(hdlg,haccel,&msg))
				continue;
		}
		if(!IsDialogMessage(hdlg,&msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////

BOOL WINAPI
Dlg_Proc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	BYTE
		*errMsg,
		buffer[ BUFFER_LENMAX ];
	int rc,errPos;
	LOGFONT	lf;
	static HWND hgrip=0;

	switch (uMsg){
	case WM_HELP:
		{
			static int help_visible=FALSE;
			if(!help_visible){
				help_visible=TRUE;
				MessageBox(hwnd,"suffix i=binary,o=octal,t=decimal,h=hex\n"
					"e.g. 011010i\n"
					"not case sensitive\n\n"
					"ALT+HOME=on top\n"
					"F5=64 bit","HELP",MB_OKCANCEL);
				help_visible=FALSE;
			}
		}
		break;
	case WM_INITDIALOG:
		hgrip=create_grippy(hwnd);
		init_main_win_anchor(hwnd);
		SendMessage(hwnd, WM_SETICON, TRUE,  (LPARAM)LoadIcon((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),MAKEINTRESOURCE(IDI_ICON)));
		SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM)LoadIcon((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),MAKEINTRESOURCE(IDI_ICON)));
		CheckDlgButton( hwnd, IDC_HEX_BUTTON, BST_CHECKED );
		GetObject( GetStockObject( ANSI_FIXED_FONT ), sizeof( lf ), &lf ); 
		lf.lfHeight=18;
		lf.lfWidth=8;
		SetWindowFont( GetDlgItem( hwnd, IDC_INPUT ), CreateFontIndirect( &lf ), TRUE );
		SetWindowFont( GetDlgItem( hwnd, IDC_ERROR ), CreateFontIndirect( &lf ), TRUE );
		return( TRUE );
	case WM_SIZE:
		resize_win(hwnd);
		grippy_move(hwnd,hgrip);
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
			case IDCANCEL:
				PostQuitMessage(0);
				break;
			case IDC_64BIT:
				{
					if(HIWORD(wParam))
						CheckDlgButton(hwnd,IDC_64BIT,!IsDlgButtonChecked(hwnd,IDC_64BIT));
				}
			case IDOK:
				{
					int is64bit;
					is64bit=IsDlgButtonChecked(hwnd,IDC_64BIT);
					ComboBox_GetText( GetDlgItem( hwnd, IDC_INPUT ), buffer, sizeof( buffer ) );
					ComboBox_AddString( GetDlgItem( hwnd, IDC_INPUT ), buffer );
					
					if(is64bit){
						__int64 result64;
						if( rc=Calculator64( buffer, &result64, &errPos, &errMsg ) )
							SetDlgItemText( hwnd, IDC_ERROR, ErrorLine( errPos, errMsg ) );
						else{
							SetDlgItemText( hwnd, IDC_ERROR, FString ( "" ) );
							SetDlgItemText( hwnd, IDC_SIGNED, FString( "%I64d", result64 ) );
							SetDlgItemText( hwnd, IDC_UNSIGNED, FString( "%I64u", result64 ) );
							SetDlgItemText( hwnd, IDC_BINARY, ToBinary64( result64 ) );
							SetDlgItemText( hwnd, IDC_HEX, FString( "%.16I64X", result64 ) );
							SetDlgItemText( hwnd, IDC_STRING, FString( "\"%s\"", CharsShow( (BYTE*)&result64, -8 ) ) );
							ShowTitle(hwnd);
						}
					}
					else{
						long result=0;
						if( rc=Calculator( buffer, &result, &errPos, &errMsg ) )
							SetDlgItemText( hwnd, IDC_ERROR, ErrorLine( errPos, errMsg ) );
						else{
							SetDlgItemText( hwnd, IDC_ERROR, FString ( "" ) );
							SetDlgItemText( hwnd, IDC_SIGNED, FString( "%ld", result ) );
							SetDlgItemText( hwnd, IDC_UNSIGNED, FString( "%lu", result ) );
							SetDlgItemText( hwnd, IDC_BINARY, ToBinary( result ) );
							SetDlgItemText( hwnd, IDC_HEX, FString( "%.8lX", result ) );
							SetDlgItemText( hwnd, IDC_STRING, FString( "\"%s\"", CharsShow( (BYTE*)&result, -4 ) ) );
							ShowTitle(hwnd);
						}
					}
				}
				break;
				
			case IDC_ONTOP:
				if(HIWORD(wParam)){
					CheckDlgButton(hwnd,IDC_ONTOP,!IsDlgButtonChecked(hwnd,IDC_ONTOP));
				}
				SetWindowPos( hwnd, IsDlgButtonChecked( hwnd, LOWORD(wParam) )? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
				break;
				
			case IDC_SIGNED_BUTTON:
			case IDC_UNSIGNED_BUTTON:
			case IDC_BINARY_BUTTON:
			case IDC_HEX_BUTTON:
			case IDC_STRING_BUTTON:
				if( IsDlgButtonChecked( hwnd, LOWORD(wParam) ) == BST_CHECKED ){
					iTitle=ID2Index( LOWORD(wParam) );
					ShowTitle(hwnd );
				}
				break;
		}
		break;
	}
	return( FALSE );
}

////////////////////////////////////////////////////////////

void ShowTitle(HWND hwnd)
{
	struct MAP{
		int button;
		int text;
	};
	struct MAP ctrl_list[]={
		{IDC_HEX_BUTTON,IDC_HEX},
		{IDC_SIGNED_BUTTON,IDC_SIGNED},
		{IDC_UNSIGNED_BUTTON,IDC_UNSIGNED},
		{IDC_BINARY_BUTTON,IDC_BINARY},
		{IDC_STRING_BUTTON,IDC_STRING}
	};
	int i,which=0;
	for(i=0;i<sizeof(ctrl_list)/sizeof(struct MAP);i++){
		if(IsDlgButtonChecked(hwnd,ctrl_list[i].button)){
			char txt[256]={0};
			GetDlgItemText(hwnd,ctrl_list[i].text,txt,sizeof(txt));
			SetWindowText(hwnd,txt);
			break;
		}
	}
}

////////////////////////////////////////////////////////////

BYTE
 *ErrorLine( int errPos, BYTE *msgErr )
{
 if( (int)strlen( msgErr ) < errPos )
   return( FString( "%-*s^", errPos, msgErr ) );
  else
   return( FString( "%*s^%s", errPos, "", msgErr ) );
}

////////////////////////////////////////////////////////////

int
 ID2Index( UINT id )
{
 if( id == IDC_SIGNED_BUTTON   ) return( TITLE_SIGNED );
 if( id == IDC_UNSIGNED_BUTTON ) return( TITLE_UNSIGNED );
 if( id == IDC_BINARY_BUTTON   ) return( TITLE_BINARY );
 if( id == IDC_HEX_BUTTON      ) return( TITLE_HEX );
 if( id == IDC_STRING_BUTTON   ) return( TITLE_STRING );
 return( TITLE_NONE );
}

////////////////////////////////////////////////////////////

BYTE
 *ToBinary( DWORD result )
{
 BYTE
  *s=temp;
 DWORD
  mask=0x80000000;

 while( mask ){
   *s++=( result & mask )? '1' : '0';
   mask=mask>>1;
   if( mask == 0x800000 || mask == 0x8000 || mask == 0x80 )
    *s++='-';
 }
 *s=0;
 return( temp );
}

BYTE
 *ToBinary64( unsigned __int64 result )
{
 BYTE *s=temp;
 unsigned __int64 mask=0x8000000000000000;
 int count=0;

 while( mask ){
   *s++=( result & mask )? '1' : '0';
   mask=mask>>1;
   count++;
   if(count>=8 && mask){
		*s++='-';
		count=0;
   }
 }
 *s=0;
 return( temp );
}

////////////////////////////////////////////////////////////

int
 Printable( BYTE ch )
{
 return( ch > 0 && ch != 9 && ch != 10 && ch != 13 );
}

////////////////////////////////////////////////////////////

BYTE
 *CharsShow( BYTE *a, int n )
{
 BYTE
  *p;
 int
   i;

 temp2[ abs( n )*2 ]=0;
 if( n < 0 )
   for( p=temp2-n, i=-n; i--; a++ )
     *(--p)=Printable( *a )? *a : ' ';
  else
   for( p=temp2, i=0; i++ < n; a++ )
     *p++=Printable( *a )? *a : ' ';
 return( temp2 );
}

/////////////////////////////////////////////////////////////

BYTE
 *FString( BYTE *format, ... )
{
 va_list
  arg;

 va_start( arg, format );
 vsprintf( temp, format, arg );
 va_end( arg );
 return( temp );
}

/// End of module //////////////////////////////////////////
