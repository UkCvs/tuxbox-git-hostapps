//
//  TuxVision
//  
//  Rev.0.0 Bernd Scharping 
//  bernd@transputer.escape.de
//
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <windows.h>
#include <windowsx.h>
#include <winbase.h>
#include <commctrl.h>
#include <shlobj.h>
#include <prsht.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vfw.h>
#include <wingdi.h>

#include <malloc.h>
#include <mmsystem.h>

#include <streams.h>
#include <strmif.h>

#include <initguid.h>

#include "guids.h"
#include "Dshow.h"
#include "TuxVision.h"
#include "resource.h"
#include "options.h"
#include "AX_Helper.h"
#include "WinHelper.h"
#include "Registry.h"
#include "..\\capture\\interface.h"
#include "..\\render\\interface.h"
#include "debug.h"

// ------------------------------------------------------------------------
// Global Stuff
// ------------------------------------------------------------------------
HINSTANCE	  ghInstApp=NULL;
HWND		  ghWndApp=NULL;
HWND		  ghWndVideo=NULL;
RecorderState gState=StateUninitialized;
BOOL		  gIsWindowMaximized=FALSE;
RECT		  gRestoreRect={0,0,0,0};
long		  gCurrentChannel=0;
long		  gNumChannels=0;
TCHAR         gszDestinationFolder[264]="";
TCHAR         gszDestinationFile[264]="";
long          gUseDeInterlacer=0;
long          gIs16By9=FALSE;
long          gRecNoPreview=FALSE;
__int64       gFilteredVideoBitrate=0;
__int64       gFilteredAudioBitrate=0;

// ------------------------------------------------------------------------
// Basic Defines
// ------------------------------------------------------------------------
#define APPNAME		"TuxVision"  // dependency to CLASS statement in resource !!!
#define MUTEXNAME	"TuxVision_MUTEX"
// ------------------------------------------------------------------------
// Forward Declarations
// ------------------------------------------------------------------------
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProcVideo (HWND hwnd, UINT message , WPARAM wParam, LPARAM lParam);
int		OnWM_Command(HWND hwnd, UINT message , WPARAM wParam, LPARAM lParam);
BOOL	DoesInstanceExist(void);
int		UpdateWindowState(HWND hWnd);
int     CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
void    CreateChannelList(HWND hwnd);
HRESULT SetTVChannel(unsigned long channel);
HRESULT UpdateChannelInfo(IDBOXIICapture *pIDBOXIICapture, __int64 currentChannel);
void    ComposeCaptureFileName(LPSTR szFileName);
void    LoadParameter(void);
void    SaveParameter(void);

// ------------------------------------------------------------------------
//
// ------------------------------------------------------------------------
BOOL  DoesInstanceExist(void)
{
    HANDLE hMutex;
    if ( hMutex = OpenMutex (MUTEX_ALL_ACCESS, FALSE, MUTEXNAME) )
        {
        CloseHandle(hMutex);
        return TRUE;
        }
    else      // if it does not exist, we create the mutex
       hMutex = CreateMutex (NULL, FALSE, MUTEXNAME);
    return FALSE;
}

// ------------------------------------------------------------------------
//
// ------------------------------------------------------------------------
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	MSG msg;
	HWND hwnd;
	WNDCLASSEX wndclassex;
	RECT rect1;
	int topV,leftV,widthV,heightV;
	int retval;
	HRESULT hr;
    HBRUSH hbr=CreateSolidBrush(0x000000);

// ------------------------------------------------------------------------
	if (DoesInstanceExist())
		return(-1);
// ------------------------------------------------------------------------
    // OLE subsystem requires applications to initialize things first!
    CoInitialize(NULL);
// ------------------------------------------------------------------------
    ghInstApp=hInstance;

	wndclassex.style		= CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS ;
	wndclassex.lpfnWndProc	= WndProc;
	wndclassex.cbClsExtra	= 0;
	wndclassex.cbWndExtra	= DLGWINDOWEXTRA;
	wndclassex.hInstance	= hInstance;
	wndclassex.hIcon		= LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON));
	wndclassex.hIconSm		= LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON+1));
	wndclassex.hCursor		= LoadCursor(hInstance,IDC_ARROW);
	wndclassex.hbrBackground= (HBRUSH)COLOR_WINDOW;
	wndclassex.lpszMenuName	= NULL;
	wndclassex.lpszClassName= APPNAME;
	wndclassex.cbSize		= sizeof(WNDCLASSEX);
	retval=RegisterClassEx(&wndclassex);

	wndclassex.style		= CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS ;
	wndclassex.lpfnWndProc	= WndProcVideo;
	wndclassex.cbClsExtra	= 0;
	wndclassex.cbWndExtra	= 0;
	wndclassex.hInstance	= hInstance;
	wndclassex.hIcon		= NULL;
	wndclassex.hIconSm		= NULL;
	wndclassex.hCursor		= NULL;
	wndclassex.hbrBackground= (HBRUSH)hbr; //(COLOR_WINDOW+1);
	wndclassex.lpszMenuName	= NULL;
	wndclassex.lpszClassName= "VideoWindow";
	wndclassex.cbSize		= sizeof(WNDCLASSEX);
	retval=RegisterClassEx(&wndclassex);

// ------------------------------------------------------------------------
    InitCommonControls();
// ------------------------------------------------------------------------
	LoadParameter();
// ------------------------------------------------------------------------
    hwnd=CreateDialog(hInstance,MAKEINTRESOURCE(MAIN),0,(DLGPROC)NULL);
    ghWndApp=hwnd;
    long stime=timeGetTime();
// ------------------------------------------------------------------------
    DisplaySplash();
// ------------------------------------------------------------------------
	hr=OpenInterface(ghWndVideo, ghInstApp);
// ------------------------------------------------------------------------
    long dtime=max(min((SPLASHTIME+stime-timeGetTime()),SPLASHTIME),0);
    dprintf("Additional SplashDelay:%d ms",dtime);
    {
    int i;
    for(i=0;i<50;i++)
        {
        if (PeekMessage(&msg, (HWND) NULL, 0, 0, PM_REMOVE))
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }
        Sleep(dtime/50);
        }
    }
    if (ghwndSplash)
        DestroyWindow(ghwndSplash);
// ------------------------------------------------------------------------
	GetWindowRect(GetDlgItem(hwnd,IDC_VIDEO_FRAME), &rect1);
	topV=rect1.top+GetSystemMetrics(SM_CYFRAME)-GetSystemMetrics(SM_CYCAPTION);
	leftV=rect1.left-GetSystemMetrics(SM_CXEDGE);
	widthV=rect1.right-rect1.left - GetSystemMetrics(SM_CXEDGE);
	heightV=rect1.bottom-rect1.top - 2*GetSystemMetrics(SM_CYFRAME);
	ghWndVideo=CreateWindow("VideoWindow",
							NULL,
							WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE, 
							leftV,
							topV,
							widthV,
							heightV,
							ghWndApp,
							NULL,
							hInstance,
							NULL);
    
    gRestoreRect.left=leftV;
    gRestoreRect.top=topV;
    gRestoreRect.right=leftV+widthV;
    gRestoreRect.bottom=topV+heightV;

    CenterWindow(ghWndApp);
	ShowWindow(ghWndApp,SW_SHOW);
	ShowWindow(ghWndVideo,SW_SHOW);
    SetWindowPos(ghWndVideo,ghWndApp,leftV,topV,0,0,SWP_NOSIZE);
// ------------------------------------------------------------------------
	if (SUCCEEDED(hr))
		{
        gState=StateStopped;
		UpdateWindowState(hwnd);
        SetWindowText(GetDlgItem(ghWndApp,IDC_DESTINATION_NAME), gszDestinationFolder);
        SendMessage( GetDlgItem(hwnd,IDC_DEINTERLACE), BM_SETCHECK, gUseDeInterlacer, 0 );
        SendMessage( GetDlgItem(hwnd,IDC_16BY9), BM_SETCHECK, gIs16By9, 0 );
        SendMessage( GetDlgItem(hwnd,IDC_NOPREVIEW), BM_SETCHECK, gRecNoPreview, 0 );


        SetTimer(ghWndApp,1,1000,NULL);
    
        CreateChannelList(ghWndApp);

		while(GetMessage(&msg,NULL,0,0))
			{
			TranslateMessage((LPMSG)&msg);
			DispatchMessage((LPMSG)&msg);
			}
		}
// ------------------------------------------------------------------------
	CloseInterface(ghWndVideo);
// ------------------------------------------------------------------------
	SaveParameter();
// ------------------------------------------------------------------------
    // Finished with OLE subsystem
    CoUninitialize();
// ------------------------------------------------------------------------
	return (msg.wParam);
// ------------------------------------------------------------------------
}

// ------------------------------------------------------------------------
// main-window handler
// ------------------------------------------------------------------------
LRESULT CALLBACK WndProc (HWND hwnd, UINT message , WPARAM wParam, LPARAM lParam)
{
	switch(message)
		{
		case WM_CREATE:
			return (0);

		case WM_COMMAND:
			OnWM_Command(hwnd,message,wParam,lParam);
			break;
		case WM_KEYDOWN:
			dprintf("KeyDown: WParam: %ld, LPARAM:%ld",wParam,lParam);
			switch(wParam)
				{
                case 27: //!!BS Escape Key
					SendMessage(ghWndVideo,message,wParam,lParam);
                    return(0);
				case 35:
				case 36:
				case 38:
				case 40:
					SendMessage(GetDlgItem(hwnd,IDC_CHANNEL),message,wParam,lParam);
					return(0);
				}
			break;

        case WM_SYSCOMMAND:
            if (wParam==SC_MAXIMIZE)
                {
                SendMessage(ghWndVideo, message, wParam, lParam);
                return(0);
                }
            break;

		case WM_LBUTTONDOWN:
		    if (!gIsWindowMaximized)
				{
				RECT rc;
				int xPos, yPos;
				GetWindowRect (hwnd, &rc);
				xPos = (rc.left+rc.right)/2;
				yPos = rc.top+1;
      			dprintf("MainWindow LButtonDown");
   				return (DefWindowProc (hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(xPos, yPos)));
				}
            else
                {
      			dprintf("Forwarding LButtonDown");
                SendMessage(ghWndVideo,WM_KEYDOWN,27,0);
                }
		  break;
		case WM_USER:
			//OnWM_User(hwnd,message,wParam,lParam);
			break;

        case WM_NCLBUTTONDBLCLK:
            dprintf("WM_NCLBUTTONDBLCLK: %ld",wParam);
            return(0);

        case WM_TIMER:
            {
            char szString[264];
            char szString2[264];
            lstrcpy(szString,"");
            if ((wParam==1) && (gState==StateRecord))
                {
                __int64 val=0;
                HRESULT hr=GetCaptureFileSize(&val);
                if (SUCCEEDED(hr))
                    {
                    val=val/(1024*1024);
                    ltoa((long)val, szString, 10);
                    lstrcat(szString," MByte");
                    }
                }
            SetDlgItemText(ghWndApp,IDC_CAPFILESIZE, szString);

            lstrcpy(szString,"0");
            if ((wParam==1) && ((gState==StateRecord)||(gState==StatePreview)))
                {
                __int64 val=0;
                HRESULT hr=GetResyncCount(&val);
                if (SUCCEEDED(hr))
                    {
                    val=val/(1024*1024);
                    ltoa((long)val, szString, 10);
                    }
                }
            SetDlgItemText(ghWndApp,IDC_RESYNCCOUNT, szString);

            lstrcpy(szString,"0 kBit/s");
            lstrcpy(szString2,"0 kBit/s");
            if ((wParam==1) && ((gState==StateRecord)||(gState==StatePreview)))
                {
                __int64 val =0;
                __int64 val2=0;
                HRESULT hr=GetCurrentBitrates(&val, &val2);
                if (SUCCEEDED(hr))
                    {
                    gFilteredAudioBitrate=(gFilteredAudioBitrate*8+val*2)/10;
                    gFilteredVideoBitrate=(gFilteredVideoBitrate*8+val2*2)/10;

                    val=gFilteredAudioBitrate/(1024);
                    ltoa((long)val, szString, 10);
                    lstrcat(szString," kBit/s");

                    val2=gFilteredVideoBitrate/(1024);
                    ltoa((long)val2, szString2, 10);
                    lstrcat(szString2," kBit/s");
                    }
                }
            else
                {
                gFilteredVideoBitrate=0;
                gFilteredAudioBitrate=0;
                }
            SetDlgItemText(ghWndApp,IDC_AUDIOBITRATE, szString);
            SetDlgItemText(ghWndApp,IDC_VIDEOBITRATE, szString2);
            }
            break;

		case WM_CLOSE:
            KillTimer(hwnd,1);
			PostQuitMessage(0);
			return (0);
		}
	return (DefWindowProc(hwnd, message, wParam, lParam));
}

// ------------------------------------------------------------------------
// video-window handler
// ------------------------------------------------------------------------
LRESULT CALLBACK WndProcVideo (HWND hwnd, UINT message , WPARAM wParam, LPARAM lParam)
{
	switch(message)
		{
//		case WM_CREATE:
//			return (0);
//		case WM_COMMAND:
//			break;
		case WM_GRAPHNOTIFY:
			dprintf("FilterGraph: Message:%ld, wParam:%ld, lParam:%ld",message,wParam,lParam);
			switch(wParam)
				{
				case EC_ERRORABORT:
				case EC_USERABORT:
				case EC_COMPLETE:
					if (gState==StatePlayback)
						{
						if (gIsWindowMaximized)
							{
							dprintf("Restore from Fullscreen");
							SetFullscreen(ghWndApp, hwnd, &gRestoreRect, FALSE);
							gIsWindowMaximized=FALSE;
							}
						//StopPlayback(ghWndVideo, &gState);
						PostMessage(ghWndApp,WM_COMMAND,IDC_STOP,0);
						}
					break;
				}
			break;

		case WM_KEYDOWN:
			dprintf("VideoWindow KeyDown");
            if ((gIsWindowMaximized)&&(wParam==27))
                {
                dprintf("Restore from Fullscreen");
                SetFullscreen(ghWndApp, hwnd, &gRestoreRect, FALSE);
                gIsWindowMaximized=FALSE;
                return(0);
                }
			break;

		case WM_LBUTTONDOWN:
		  if (!gIsWindowMaximized)
            {
  			dprintf("VideoWindow LButtonDown");
			SendMessage(ghWndApp,message,wParam,lParam);
            }
          else  
            {
            dprintf("Restore from Fullscreen");
            SetFullscreen(ghWndApp, hwnd, &gRestoreRect, FALSE);
            gIsWindowMaximized=FALSE;
            }
		  break;

        case WM_SYSCOMMAND:
            if (wParam==SC_MAXIMIZE)
                {
                if (!gIsWindowMaximized)
                    {
                    dprintf("Switching to Fullscreen");
                    SetFullscreen(NULL, hwnd, &gRestoreRect, TRUE);
                    gIsWindowMaximized=TRUE;
                    }
                return(0);
                }
            else 
            if (wParam==SC_RESTORE)
                {
                if (gIsWindowMaximized)
                    {
                    dprintf("Restore from Fullscreen");
                    SetFullscreen(ghWndApp, hwnd, &gRestoreRect, FALSE);
                    gIsWindowMaximized=FALSE;
                    }
                return(0);
                }
            break;

        case WM_LBUTTONDBLCLK:
            //if (gState==StatePlayback)
                {
                if (gIsWindowMaximized)
                    {
                    dprintf("Restore from Fullscreen");
                    SetFullscreen(ghWndApp, hwnd, &gRestoreRect, FALSE);
                    gIsWindowMaximized=FALSE;
                    }
                else
                    {
                    dprintf("Switching to Fullscreen");
                    SetFullscreen(NULL, hwnd, &gRestoreRect, TRUE);
                    gIsWindowMaximized=TRUE;
                    }
                }
            break;
		case WM_USER:
			break;
		case WM_CLOSE:
			PostQuitMessage(0);
			return (0);
		}
	return (DefWindowProc(hwnd, message, wParam, lParam));
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch(uMsg) 
		{
		case BFFM_INITIALIZED: 
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)gszDestinationFolder);
			break;
		}
	return(0);
}

int OnWM_Command(HWND hwnd, UINT message , WPARAM wParam, LPARAM lParam)
{
	switch(GET_WM_COMMAND_ID (wParam, lParam))
		{
        case IDC_GETCHANNELLIST:
            CreateChannelList(hwnd);
            break;

        case IDC_DESTINATION:
			{
            TCHAR buffer[_MAX_PATH];
            BOOL ret;
            LPITEMIDLIST pil;
            BROWSEINFO bi;

	        lstrcpy(buffer, gszDestinationFolder);

            bi.hwndOwner=hwnd; 
            bi.pidlRoot=NULL; 
            bi.pszDisplayName=buffer; 
            bi.lpszTitle=""; 
            bi.ulFlags=0;
            bi.lpfn=BrowseCallbackProc; //NULL; 
            bi.lParam=0; 
            bi.iImage=0; 

	        LPMALLOC pMalloc;

            if (SHGetMalloc(&pMalloc) == NOERROR)
                {
                pil=SHBrowseForFolder(&bi);
                if (pil!=NULL)
                    {
                    ret=SHGetPathFromIDList(pil,gszDestinationFolder);
		            pMalloc->Free(pil);
                    }
		        pMalloc->Release();
                }
            SetWindowText(GetDlgItem(hwnd,IDC_DESTINATION_NAME), gszDestinationFolder);
            }
            break;

		case IDC_RECORD:
            CreateCaptureGraph();
            gState=StateRecord;
            RunGraph(gpIGraphBuilder);
			UpdateWindowState(hwnd);
			break;

		case IDC_STOP:
            StopGraph(gpIGraphBuilder);
            DestroyGraph(gpIGraphBuilder);
            gState=StateStopped;
			UpdateWindowState(hwnd);
			break;

		case IDC_PLAY:
            gIsWindowMaximized=FALSE;
            gState=StatePlayback;
			UpdateWindowState(hwnd);
			break;

		case IDC_PREVIEW:
            CreatePreviewGraph();
            gState=StatePreview;
            RunGraph(gpIGraphBuilder);
            UpdateWindowState(hwnd);
            break;

		case IDC_OPTIONS:
			CreatePropertySheet(ghWndApp, ghInstApp, 0);
            CreateChannelList(ghWndApp);
			UpdateWindowState(hwnd);
			break;

		case IDC_DEINTERLACE:
            gUseDeInterlacer=SendMessage( GetDlgItem(hwnd,IDC_DEINTERLACE), BM_GETCHECK, 0, 0 );
            SetDeInterlacerState(gUseDeInterlacer);
			break;

		case IDC_16BY9:
            {
            RECT rc;
            gIs16By9=SendMessage( GetDlgItem(hwnd,IDC_16BY9), BM_GETCHECK, 0, 0 );
            GetClientRect(ghWndVideo, &rc);
            ConnectVideoWindow(gpIGraphBuilder, ghWndVideo, &rc, gIs16By9);
            }
			break;

		case IDC_NOPREVIEW:
            gRecNoPreview=SendMessage( GetDlgItem(hwnd,IDC_NOPREVIEW), BM_GETCHECK, 0, 0 );
			break;

		case IDC_EXIT:
            StopGraph(gpIGraphBuilder);
            DestroyGraph(gpIGraphBuilder);
			SendMessage(hwnd,WM_CLOSE,0,0);
			break;
// --------------------------------------------------------------------------
		case IDC_CHANNEL:
			if (GET_WM_COMMAND_CMD (wParam, lParam)==CBN_SELCHANGE)
				{
				unsigned long channel=0;
				int sel=0;
				sel=SendMessage( GetDlgItem(hwnd,IDC_CHANNEL), CB_GETCURSEL, 0, 0 );
				channel=SendMessage( GetDlgItem(hwnd,IDC_CHANNEL), CB_GETITEMDATA, sel, 0 );
				SetTVChannel(channel);
				gCurrentChannel=sel;
				}
			break;
// --------------------------------------------------------------------------
		}
	return(0);
}


int UpdateWindowState(HWND hWnd)
{
	switch(gState)
		{
		case StateStopped:
			EnableWindow(GetDlgItem(hWnd,IDC_PREVIEW),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_STOP),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_OPTIONS),TRUE);
            EnableWindow(GetDlgItem(hWnd,IDC_DESTINATION),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHANNEL),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_GETCHANNELLIST),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_RECORD),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_NOPREVIEW),TRUE);
//			EnableWindow(GetDlgItem(hWnd,IDC_PLAY),TRUE);

			break;
		case StatePreview:
			EnableWindow(GetDlgItem(hWnd,IDC_PREVIEW),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_STOP),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_OPTIONS),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_DESTINATION),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHANNEL),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_GETCHANNELLIST),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_RECORD),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_NOPREVIEW), FALSE);
//			EnableWindow(GetDlgItem(hWnd,IDC_PLAY),FALSE);
			break;
		case StateRecord:
			EnableWindow(GetDlgItem(hWnd,IDC_PREVIEW),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_STOP),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_OPTIONS),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_DESTINATION),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHANNEL),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_GETCHANNELLIST),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_RECORD),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_NOPREVIEW), FALSE);
//			EnableWindow(GetDlgItem(hWnd,IDC_PLAY),FALSE);
			break;
		case StatePlayback:
			EnableWindow(GetDlgItem(hWnd,IDC_PREVIEW),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_STOP),TRUE);
			EnableWindow(GetDlgItem(hWnd,IDC_OPTIONS),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_DESTINATION),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHANNEL),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_GETCHANNELLIST),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_RECORD),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_NOPREVIEW), FALSE);
//			EnableWindow(GetDlgItem(hWnd,IDC_PLAY),FALSE);
			break;
		}
	UpdateWindow(hWnd);
	return(0);
}

void CreateChannelList(HWND hwnd)
{
    long index=0;
    HRESULT hr=E_FAIL;
    SendMessage( GetDlgItem(hwnd,IDC_CHANNEL), CB_RESETCONTENT , 0, 0 );
    if (gpVCap!=NULL)
        {
        IDBOXIICapture *pIDBOXIICapture=NULL;
        hr=gpVCap->QueryInterface(IID_IDBOXIICapture, (void **)&pIDBOXIICapture);
        if (SUCCEEDED(hr))
            {
            int i;
            char buf[264];
            __int64 count=-1;
            __int64 currentChannel=0;

            SetDlgItemText(ghWndApp,IDC_CHANNELINFO, "");

            hr=pIDBOXIICapture->getParameter(CMD_GETCHANNELLIST, (__int64*)buf, &count);
            if (SUCCEEDED(hr))
                hr=pIDBOXIICapture->getParameter(CMD_GETCHANNEL, (__int64*)&currentChannel,NULL);
            else
                {
                RELEASE(pIDBOXIICapture);
                return;
                }
            if (SUCCEEDED(hr)&&(count>=0))
                {
                hr=UpdateChannelInfo(pIDBOXIICapture, currentChannel);

                for(i=0;i<=count;i++)
                    {
                    __int64 cid=i;
                    hr=pIDBOXIICapture->getParameter(CMD_GETCHANNELLIST, (__int64*)buf, &cid);
                    if (cid<0)
                        break;
                    index=SendMessage( GetDlgItem(hwnd,IDC_CHANNEL), CB_ADDSTRING, 0, (LONG)(LPSTR)(buf) );
                    SendMessage( GetDlgItem(hwnd,IDC_CHANNEL), CB_SETITEMDATA, index, (unsigned long)cid );
                    if (cid==currentChannel)
                        SendMessage( GetDlgItem(hwnd,IDC_CHANNEL), CB_SETCURSEL, index, 0 );
                        
                    //dprintf("Update Channel: <%ld> <%s> <%ld>",i,buf,(unsigned long)cid);
                    }
                }
            RELEASE(pIDBOXIICapture);
            }
        }
}

HRESULT SetTVChannel(unsigned long channel)
{
    HRESULT hr=NOERROR;
    if (gpVCap!=NULL)
        {
        IDBOXIICapture *pIDBOXIICapture=NULL;
        hr=gpVCap->QueryInterface(IID_IDBOXIICapture, (void **)&pIDBOXIICapture);
        if (SUCCEEDED(hr))
            hr=pIDBOXIICapture->setParameter(CMD_SETCHANNEL, (__int64)channel);

        if (SUCCEEDED(hr))
            hr=UpdateChannelInfo(pIDBOXIICapture, channel);
        else
            SetDlgItemText(ghWndApp,IDC_CHANNELINFO, "");

        RELEASE(pIDBOXIICapture);
        }
    return(hr);
}

HRESULT UpdateChannelInfo(IDBOXIICapture *pIDBOXIICapture, __int64 currentChannel)
{
    char buf[264];
    char szEPGID[264];
    char szEPGDate[264];
    char szEPGTime[264];
    char szEPGLen[264];
    char szEPGTitle[264];
    char szTmp[1024];
    int len=0;
    HRESULT hr=NOERROR;

    if (pIDBOXIICapture==NULL)
        return(E_POINTER);

    SetDlgItemText(ghWndApp,IDC_CHANNELINFO, "");

    hr=pIDBOXIICapture->getParameter(CMD_GETCHANNELINFO, &currentChannel, (__int64*)buf);
    dprintf("ChannelInfo: <%s>",buf);
//    if (lstrlen(buf)==0)
//        {
//        Sleep(2000);
//        hr=pIDBOXIICapture->getParameter(CMD_GETCHANNELINFO, &currentChannel, (__int64*)buf);
//        }
    sscanf(buf,"%s %s %s %s",szEPGID, szEPGDate, szEPGTime, szEPGLen);
    lstrcpy(szTmp,szEPGID);lstrcat(szTmp," ");
    lstrcat(szTmp,szEPGDate);lstrcat(szTmp," ");
    lstrcat(szTmp,szEPGTime);lstrcat(szTmp," ");

    if (strstr(szEPGDate,".")!=NULL)
        {
        lstrcat(szTmp,szEPGLen);
        lstrcat(szTmp," ");
        }
    
    len=strlen(szTmp);
    if (len<lstrlen(buf))
        {
        lstrcpy(szEPGTitle, buf+len);
        }
    else
        lstrcpy(szEPGTitle, "");

    SetDlgItemText(ghWndApp,IDC_CHANNELINFO, szEPGTitle);
    lstrcpy(gszDestinationFile, szEPGTitle);
 
    return(hr);
}

void LoadParameter(void)
{
    char regval[264];

    lstrcpy(gszDestinationFolder,"C:\\");
    gUseDeInterlacer=0;

    if (GetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "DestinationPath", (unsigned char *)regval, sizeof(regval)))
        lstrcpy(gszDestinationFolder,regval);
    if (GetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "DeInterlace", (unsigned char *)regval, sizeof(regval)))
        gUseDeInterlacer=atoi(regval);
    if (GetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "16By9", (unsigned char *)regval, sizeof(regval)))
        gIs16By9=atoi(regval);
    if (GetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "RecNoPreview", (unsigned char *)regval, sizeof(regval)))
        gRecNoPreview=atoi(regval);
}

void SaveParameter(void)
{
    char regval[264];

    dprintf("Saving settings ...");
	CreateRegKey(HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "");

	wsprintf((char *)regval,"%s",gszDestinationFolder);
    SetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "DestinationPath", (unsigned char *)regval, lstrlen(regval));

	wsprintf((char *)regval,"%ld",gUseDeInterlacer);
    SetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "DeInterlace", (unsigned char *)regval, lstrlen(regval));

	wsprintf((char *)regval,"%ld",gIs16By9);
    SetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "16By9", (unsigned char *)regval, lstrlen(regval));

	wsprintf((char *)regval,"%ld",gRecNoPreview);
    SetRegStringValue (HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, "", "RecNoPreview", (unsigned char *)regval, lstrlen(regval));
}

