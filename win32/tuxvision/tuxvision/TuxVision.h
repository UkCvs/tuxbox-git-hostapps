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

#ifndef __TUXVISION_H__
#define __TUXVISION_H__

#define ALPHA_VERSION       1
#define ALPHA_STRING        "TuxVision"
#define SPLASHTIME          3000
#define REGISTRY_SUBKEY		"Software\\TuxVision"
#define REVISION            "Rev.0.0.0.3"
typedef enum{StateStopped, StatePreview, StateRecord, StatePlayback, StateUninitialized} RecorderState;

extern HINSTANCE		ghInstApp;
extern HWND				ghWndApp;
extern HWND				ghWndVideo;
extern TCHAR            gszDestinationFolder[264];
extern TCHAR            gszDestinationFile[264];
extern long             gUseDeInterlacer;
extern long             gIs16By9;
extern long             gRecNoPreview;

#define Width(x)   (x.right-x.left)
#define Height(x)  (x.bottom-x.top)
#define pWidth(x)   (x->right-x->left)
#define pHeight(x)  (x->bottom-x->top)


HRESULT SetupParameter(HWND hWnd, RecorderState *state);

HRESULT StartPreview(HWND hWnd, RecorderState *state);
HRESULT StopPreview(HWND hWnd, RecorderState *state);
HRESULT StartRecord(HWND hWnd, RecorderState *state);
HRESULT StopRecord(HWND hWnd, RecorderState *state);
HRESULT StartPlayback(HWND hWnd, RecorderState *state);
HRESULT StopPlayback(HWND hWnd, RecorderState *state);


HRESULT SetInput(long input);
HRESULT SetTVChannel(long channel);
HRESULT SetTVStandard(long std);

HRESULT SetFullscreen(HWND hWndParent, HWND hWnd, RECT *restore, BOOL flag);
HRESULT RedrawVideoWindow(HWND hWnd);

HRESULT GetChannelList(long *CurrentChannel,
                       long *NumChannels,
                       HWND hWnd,
                       int  idListBox);

HRESULT LoadSettings();
HRESULT SaveSettings();


#endif