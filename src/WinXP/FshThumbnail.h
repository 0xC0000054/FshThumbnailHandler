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

#include <Shlobj.h>
#include <wincodec.h>

class CFshThumbnailHandler 
	: IPersistFile,
	  IExtractImage
{
public:
	CFshThumbnailHandler();
	~CFshThumbnailHandler();

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void **ppvObject);
	STDMETHODIMP_(DWORD) AddRef();
	STDMETHODIMP_(DWORD) Release();

	// IPersist methods (via IPersistFile)
	STDMETHODIMP GetClassID(CLSID *pClassID);

	// IPersistFile methods
	STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);
	STDMETHODIMP IsDirty();

	STDMETHODIMP Load(LPCOLESTR pszFileName, 
					  DWORD dwMode);

	STDMETHODIMP Save(LPCOLESTR pszFileName, 
					  BOOL fRemember);

	STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);

	// IExtractImage methods
	STDMETHODIMP Extract(HBITMAP *phBmpImage);

	STDMETHODIMP GetLocation(LPWSTR pszPathBuffer, 
							 DWORD cchMax, 
							 DWORD *pdwPriority, 
							 const SIZE *prgSize, 
							 DWORD dwRecClrDepth, 
							 DWORD *pdwFlags);

protected:
	volatile LONG m_lRefCount;

private:
	BSTR m_bstrFileName;
	SIZE m_size;
	HRESULT CheckQFS();
	HRESULT QFSDecompress(BYTE* inData, BYTE** outData, DWORD length);
	HRESULT LoadFSH(IWICImagingFactory* factory, IWICBitmap **image);
	HRESULT CreatePalette(int offset, IWICImagingFactory *factory, IWICPalette** palette);

	BYTE* fshBytes;	
	HANDLE hFile;

};
