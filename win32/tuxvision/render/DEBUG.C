//
//  DBOXII Render Filter
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


#include "debug.h"

#if (defined DEBUG) || (defined _DEBUG)

void cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[4096];

    static BOOL fDebug = -1;

    lstrcpy(ach, "DBOXII Render: ");
    wvsprintf(ach+lstrlen(ach),szFormat,(LPVOID)(&szFormat+1));
    lstrcat(ach, "\r\n");

    OutputDebugString(ach);
}

#endif 

