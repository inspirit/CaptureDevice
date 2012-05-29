/*
 * Copyright (C) 2002 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __DSHOW_INCLUDED__
#define __DSHOW_INCLUDED__

#define AM_NOVTABLE

#ifndef __WINESRC__
# include <windows.h>
# include <windowsx.h>
#else
# include <windef.h>
# include <wingdi.h>
# include <objbase.h>
#endif
#include <olectl.h>
#include <_dshow/ddraw.h>
#include <mmsystem.h>
/* FIXME: #include <strsafe.h>*/

#ifndef NUMELMS
#define NUMELMS(array) (sizeof(array)/sizeof((array)[0]))
#endif

#include <_dshow/strmif.h>
#include <_dshow/amvideo.h>
#ifdef DSHOW_USE_AMAUDIO
/* FIXME: #include <amaudio.h>*/
#endif
#include <_dshow/control.h>
#include <_dshow/evcode.h>
#include <_dshow/uuids.h>
#include <_dshow/errors.h>
/* FIXME: #include <edevdefs.h> */
#include <_dshow/audevcod.h>
/* FIXME: #include <dvdevcod.h> */

#ifndef OATRUE
#define OATRUE (-1)
#endif
#ifndef OAFALSE
#define OAFALSE (0)
#endif

#endif /* __DSHOW_INCLUDED__ */
