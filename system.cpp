/*
Tonemaster - HDR software
Copyright (C) 2018 Stefan Monov <logixoul@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "precompiled.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "system.h"

// https://stackoverflow.com/a/875264/122687
wstring exePath() {
	TCHAR buffer[1024];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	string::size_type pos = wstring(buffer).find_last_of(L"\\/");
	return wstring(buffer).substr(0, pos);
}
