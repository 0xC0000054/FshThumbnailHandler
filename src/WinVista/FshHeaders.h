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

#pragma once

typedef struct FshHeader
{
	char SHPI[4];
	int size;
	int numBmps;
	char dirID[4];
}Fsh_Header;

typedef struct FshDirEntry
{
	char name[4];
	int offset;
}FshDir;
typedef struct FshEntryHeader
{
	int code;
	unsigned short width;
	unsigned short height;
	unsigned short misc[4];
}FshEntry;