/*
* This file is part of FshThumbnailHandler, a Windows thumbnail handler for FSH images.
*
* Copyright (c) 2009, 2010, 2012, 2013, 2023 Nicholas Hayes
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

/////////////////////////////////////////////////////////////////////////////////
// Paint.NET                                                                   //
// Copyright (C) Rick Brewster, Tom Jackson, and past contributors.            //
// Portions Copyright (C) Microsoft Corporation. All Rights Reserved.          //
// See src/Resources/Files/License.txt for full licensing and attribution      //
// details.                                                                    //
// .                                                                           //
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef PDNSHELL_EXPORTS
#define PDNSHELL_API __declspec(dllexport)
#else
#define PDNSHELL_API __declspec(dllimport)
#endif

extern HINSTANCE g_hInstance;
extern volatile LONG g_lRefCount;

#ifdef NDEBUG
#define TraceOut if(0)
#else
extern void TraceOut(const char *szFormat, ...);
#endif

#define TraceEnter() TraceOut("enter: %s", __FUNCTION__);
#define TraceLeave() TraceOut("leave: %s", __FUNCTION__);
#define TraceLeaveHr(hr) TraceOut("leave: %s, hr=0x%x", __FUNCTION__, hr);
extern const WCHAR *GuidToString(GUID guid);
