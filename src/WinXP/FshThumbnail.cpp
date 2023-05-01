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

#pragma warning (disable: 4530)
#include "FshThumbnail.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")

#include <atlenc.h>
#include "FshGuid.h"
#include "FshShell.h"
#include "FshHeaders.h"
#include "DXT.h"
#include <windows.h>


CFshThumbnailHandler::CFshThumbnailHandler()
	: m_lRefCount(1),
	  m_bstrFileName(nullptr)
{
	m_size.cx = -1;
	m_size.cy = -1;
	InterlockedIncrement(&g_lRefCount);
}

CFshThumbnailHandler::~CFshThumbnailHandler()
{
	InterlockedDecrement(&g_lRefCount);
}

DWORD CFshThumbnailHandler::AddRef()
{
	TraceEnter();
	TraceLeave();
	return (DWORD)InterlockedIncrement(&m_lRefCount);
}

DWORD CFshThumbnailHandler::Release()
{
	TraceEnter();
	DWORD dwRefCount = (DWORD)InterlockedDecrement(&m_lRefCount);

	if (0 == dwRefCount)
	{
		delete this;
	}

	TraceLeave();
	return dwRefCount;
}

STDMETHODIMP CFshThumbnailHandler::QueryInterface(REFIID iid, void **ppvObject)
{
	HRESULT hr = S_OK;
	TraceEnter();
	TraceOut("riid=%S", GuidToString(iid));

	if (nullptr == ppvObject)
	{
		return E_INVALIDARG;
	}

	if (SUCCEEDED(hr))
	{
		*ppvObject = nullptr;

		if (IsEqualCLSID(iid, IID_IUnknown))
		{
			*ppvObject = this;
		}
		else if (IsEqualCLSID(iid, IID_IPersistFile))
		{
			*ppvObject = reinterpret_cast<IPersistFile*>(this);
		}
		else if (IsEqualCLSID(iid, IID_IExtractImage))
		{
			*ppvObject = reinterpret_cast<IExtractImage*>(this);
		}
		else if (IsEqualCLSID(iid, CLSID_FshThumbnail))
		{
			*ppvObject = this;
		}
		else
		{
			hr = E_NOINTERFACE;
		}
	}

	if (SUCCEEDED(hr))
	{
		(*(IUnknown **)ppvObject)->AddRef();
		hr = S_OK;
	}

	TraceLeaveHr(hr);
	return hr;
}


STDMETHODIMP CFshThumbnailHandler::GetClassID(CLSID *pClassID)
{
	HRESULT hr = E_NOTIMPL;
	TraceEnter();
	TraceLeaveHr(hr);
	return hr;
}

STDMETHODIMP CFshThumbnailHandler::GetCurFile(LPOLESTR *ppszFileName)
{
	HRESULT hr = E_NOTIMPL;
	TraceEnter();
	TraceLeaveHr(hr);
	return hr;
}

STDMETHODIMP CFshThumbnailHandler::IsDirty()
{
	HRESULT hr = E_NOTIMPL;
	TraceEnter();
	TraceLeaveHr(hr);
	return hr;
}

STDMETHODIMP CFshThumbnailHandler::Load(LPCOLESTR pszFileName,
									  DWORD dwMode)
{
	HRESULT hr = S_OK;
	TraceEnter();

	TraceOut("filename=%S", pszFileName);
	TraceOut("mode=%u", dwMode);

	if (SUCCEEDED(hr))
	{
		if (nullptr == pszFileName)
		{
			hr = E_INVALIDARG;
		}
	}

	if (SUCCEEDED(hr))
	{
		SysFreeString(m_bstrFileName);
		m_bstrFileName = SysAllocString(pszFileName);

		if (nullptr == m_bstrFileName)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			hr = S_OK;
		}
	}

	TraceLeaveHr(hr);
	return hr;
}

STDMETHODIMP CFshThumbnailHandler::Save(LPCOLESTR pszFileName,
									  BOOL fRemember)
{
	HRESULT hr = E_NOTIMPL;
	TraceEnter();

	TraceOut("fileName=%S", pszFileName);

	TraceLeaveHr(hr);
	return hr;
}

STDMETHODIMP CFshThumbnailHandler::SaveCompleted(LPCOLESTR pszFileName)
{
	HRESULT hr = E_NOTIMPL;
	TraceEnter();
	TraceLeaveHr(hr);
	return hr;
}

// ReadFile does not guarantee that it will read all the bytes that you ask for.
// It may decide to read fewer bytes for whatever reason. This function is a
// wrapper around ReadFile that loops until all the bytes you have asked for
// are read, or there was an error, or the end of file was reached. EOF is
// considered an error condition; when you ask for N bytes with this function
// you either get all N bytes, or an error.
static HRESULT ReadFileComplete(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead)
{
	HRESULT hr = S_OK;

	while (SUCCEEDED(hr) && nNumberOfBytesToRead > 0)
	{
		DWORD dwBytesRead = 0;

		BOOL bResult = ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, &dwBytesRead, nullptr);

		if (!bResult)
		{
			DWORD dwError = GetLastError();
			hr = HRESULT_FROM_WIN32(dwError);
		}
		else if (bResult && 0 == dwBytesRead)
		{
			hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
		}
		else
		{
			lpBuffer = reinterpret_cast<BYTE*>(lpBuffer) + dwBytesRead;
			nNumberOfBytesToRead -= dwBytesRead;
		}
	}

	return hr;
}


static SIZE ComputeThumbnailSize(int originalWidth, int originalHeight, int maxEdgeLength)
{
	SIZE thumbSize;
	ZeroMemory(&thumbSize, sizeof(thumbSize));

	if (originalWidth > originalHeight)
	{
		int longSide = min(originalWidth, maxEdgeLength);
		thumbSize.cx = longSide;
		thumbSize.cy = max(1, (originalHeight * longSide) / originalWidth);
	}
	else if (originalHeight > originalWidth)
	{
		int longSide = min(originalHeight, maxEdgeLength);
		thumbSize.cx = max(1, (originalWidth * longSide) / originalHeight);
		thumbSize.cy = longSide;
	}
	else // if (docSize.Width == docSize.Height)
	{
		int longSide = min(originalWidth, maxEdgeLength);
		thumbSize.cx = longSide;
		thumbSize.cy = longSide;
	}

	return thumbSize;
}

HRESULT CFshThumbnailHandler::CheckQFS()
{
	TraceEnter();
	LARGE_INTEGER sLength;

	HRESULT hr = S_OK;

	GetFileSizeEx(hFile, &sLength);

	if (SUCCEEDED(hr))
	{
		DWORD length = sLength.LowPart;

		LARGE_INTEGER ofs = {0};
		SetFilePointerEx(hFile, ofs, nullptr, FILE_BEGIN);

		BYTE bytes[9];

		hr = ReadFileComplete(hFile, bytes, sizeof(bytes));

		if (SUCCEEDED(hr))
		{

			SetFilePointerEx(hFile, ofs, nullptr, FILE_BEGIN);

			if ((bytes[0] & 0xfe) != 0x10 || bytes[1] != 0xfb)
			{
				if ((bytes[4] & 0xfe) != 0x10 || bytes[5] != 0xfb)
				{
					fshBytes = reinterpret_cast<BYTE*>(LocalAlloc(LPTR, length));
					if (!fshBytes)
						return E_OUTOFMEMORY;

					hr = ReadFileComplete(hFile, fshBytes, length);

					return hr;
				}
			}
			// begin QFS decompression

			BYTE* qfsBytes = reinterpret_cast<BYTE*>(LocalAlloc(LPTR, length));

			if (!qfsBytes)
				return E_OUTOFMEMORY;

			hr = ReadFileComplete(hFile, qfsBytes, length);

			if (SUCCEEDED(hr))
			{
				hr = QFSDecompress(qfsBytes, &fshBytes, length);
			}

			if (qfsBytes)
			{
				LocalFree(qfsBytes);
			}
		}
	}

	TraceLeaveHr(hr);
	return hr;
}

HRESULT CFshThumbnailHandler::QFSDecompress(BYTE* inData, BYTE** outData, DWORD length)
{
	int startOffset = 0;

	if ((inData[0] & 0xfe) != 0x10 && inData[1] != 0xfb)
	{
		if ((inData[4] & 0xfe) == 0x10 && inData[5] == 0xfb)
		{
			startOffset = 4;
		}
		else
		{
			return E_FAIL;
		}
	}

	UINT32 outLength = ((inData[startOffset + 2] << 16) | (inData[startOffset + 3] << 8) | inData[startOffset + 4]);

	*outData = reinterpret_cast<BYTE*>(LocalAlloc(LPTR, outLength));
	if (!*outData)
		return E_OUTOFMEMORY;

	UINT32 index = startOffset + 5;
	if ((inData[startOffset] & 1) > 0)
	{
		index = 8;
	}

	BYTE ccbyte0 = 0; // control char 0
	BYTE ccbyte1 = 0; // control char 1
	BYTE ccbyte2 = 0; // control char 2
	BYTE ccbyte3 = 0; // control char 3

	UINT32 plainCount = 0;
	UINT32 copyCount = 0;
	UINT32 copyOffset = 0;

	UINT32 srcIndex = 0;
	UINT32 outIndex = 0;

	BYTE* compData = inData + index;

	while (index < length && outIndex < outLength) // code adapted from http://simswiki.info/wiki.php?title=DBPF_Compression
	{
		ccbyte0 = compData[index++];

		if (ccbyte0 >= 0xFC)
		{
			plainCount = (ccbyte0 & 3);

			if ((index + plainCount) > length)
			{
				plainCount = length - index;
			}

			copyCount = 0;
			copyOffset = 0;
		}
		else if (ccbyte0 >= 0xE0)
		{
			plainCount = (ccbyte0 - 0xDF) << 2;

			copyCount = 0;
			copyOffset = 0;
		}
		else if (ccbyte0 >= 0xC0)
		{
			ccbyte1 = compData[index++];
			ccbyte2 = compData[index++];
			ccbyte3 = compData[index++];

			plainCount = (ccbyte0 & 3);

			copyCount = (((ccbyte0 >> 2) & 0x03) << 8) + ccbyte3 + 5;
			copyOffset = (((ccbyte0 & 16) << 12) + (ccbyte1 << 8)) + ccbyte2 + 1;
		}
		else if (ccbyte0 >= 0x80)
		{
			ccbyte1 = compData[index++];
			ccbyte2 = compData[index++];

			plainCount = (ccbyte1 >> 6) & 0x03;

			copyCount = (ccbyte0 & 0x3F) + 4;
			copyOffset = ((ccbyte1 & 0x3F) << 8) + ccbyte2 + 1;
		}
		else
		{
			ccbyte1 = compData[index++];

			plainCount = (ccbyte0 & 3);

			copyCount = ((ccbyte0 & 0x1c) >> 2) + 3;
			copyOffset = ((ccbyte0 >> 5) << 8) + ccbyte1 + 1;
		}

		for (UINT32 i = 0; i < plainCount; i++)
		{
			compData[index] = *outData[outIndex];
			index++;
			outIndex++;
		}

		srcIndex = outIndex - copyOffset;

		for (UINT32 i = 0; i < copyCount; i++)
		{
			*outData[srcIndex] = *outData[outIndex];
			srcIndex++;
			outIndex++;
		}
	}

	return S_OK;
}

HRESULT CFshThumbnailHandler::CreatePalette(int offset, IWICImagingFactory *factory, IWICPalette** palette)
{
	FshEntryHeader * palHdr = (FshEntryHeader*)(fshBytes + offset);

	int code = palHdr->code & 255;
	WICColor* colors = reinterpret_cast<WICColor*>(LocalAlloc(LPTR, (palHdr->width * sizeof(WICColor))));

	if (colors == nullptr)
		return E_OUTOFMEMORY;

	BYTE* p = (fshBytes + (offset + 16));
	USHORT* sPtr = reinterpret_cast<USHORT*>(p);
	UINT32* iPtr = reinterpret_cast<UINT32*>(p);

	const UINT32 OpaqueAlphaMask = 0xFF000000;

	switch (code) // WIC palettes appear to be BGRA order
	{
		case 0x22: // 24-bit DOS palette RGB (8:8:8)
			for (int i = 0; i < palHdr->width; i++)
			{
				colors[i] = (((p[0] << 16) + (p[1] << 8) + p[2]) << 2) | OpaqueAlphaMask;
				p += 3;
			}
			break;
		case 0x24: // 24-bit palette RGB (8:8:8)
			for (int i = 0; i < palHdr->width; i++)
			{
				colors[i] = ((p[0] << 16) + (p[1] << 8) + p[2]) | OpaqueAlphaMask;
				p += 3;
			}
			break;
		case 0x29: // 16-bit NFS5 palette RGAB (5:5:1:5)
			for (int i = 0; i < palHdr->width; i++)
			{
				colors[i] = (((*sPtr & 0x1f) + (((*sPtr >> 5) & 0x3f) << 7) + (((*sPtr >> 11) & 0x1f) << 16)) << 3);

				if ((*sPtr & 0x20) > 0)
				{
					colors[i] |= OpaqueAlphaMask;
				}

				sPtr++;
			}
			break;
		case 0x2a: // 32-bit palette ARGB (8:8:8:8)
			for (int i = 0; i < palHdr->width; i++)
			{
				colors[i] = *iPtr;
				iPtr++;
			}
			break;
		case 0x2d: // 16-bit palette ARGB (1:5:5:5)
			for (int i = 0; i < palHdr->width; i++)
			{
				colors[i] = (((*sPtr & 0x1f) + (((*sPtr >> 5) & 0x3f) << 8) + (((*sPtr >> 10) & 0x1f) << 16)) << 3);

				if ((*sPtr & 0x8000) > 0)
				{
					colors[i] |= OpaqueAlphaMask;
				}

				sPtr++;
			}
			break;
	}

	HRESULT hr = factory->CreatePalette(palette);
	if (SUCCEEDED(hr))
	{
		IWICPalette *tmp = *palette;
		hr = tmp->InitializeCustom(colors, static_cast<UINT32>(palHdr->width));
	}

	if (colors)
	{
		LocalFree(colors);
	}


	return hr;
}

static bool CheckFshSig (char identifier[])
{
	return identifier[0] == 'S' &&
		   identifier[1] == 'H' &&
		   identifier[2] == 'P' &&
		   identifier[3] == 'I';
}

static UINT GetImageDataSize(int width, int height, int code)
{
	UINT size = 0;
	switch (code)
	{
		case 0x7d:
		case 0x60:
		case 0x61:
		case 0x6d:
		case 0x7e:
			size = (width * height) * 4;
			break;
		case 0x7f:
		case 0x78:
			size = (width * height) * 3;
			break;
		case 0x7b:
			size = (width * height);
			break;
	}

	return size;
}

static HRESULT LockBitmap(IWICBitmap* image, IWICBitmapLock** lock, BYTE** scan0, UINT32* stride)
{
	UINT width, height;
	HRESULT hr = image->GetSize(&width, &height);

	if (SUCCEEDED(hr))
	{
		WICRect rect = {0 , 0, width, height };
		hr = image->Lock(&rect, WICBitmapLockWrite, lock);
		if (SUCCEEDED(hr))
		{
			UINT32 bufferSize;
			IWICBitmapLock* tmp = *lock;

			hr = tmp->GetStride(stride);
			if (SUCCEEDED(hr))
			{
				hr = tmp->GetDataPointer(&bufferSize, reinterpret_cast<WICInProcPointer*>(*scan0));
			}
		}
	}

	return hr;
}

HRESULT CFshThumbnailHandler::LoadFSH(IWICImagingFactory *factory, IWICBitmap **image)
{
	image = nullptr;

	TraceEnter();

	HRESULT hr = CheckQFS();
	if (SUCCEEDED(hr))
	{
		FshHeader *head = reinterpret_cast<FshHeader*>(fshBytes);

		if (!CheckFshSig(head->SHPI))
		{
			return E_FAIL;
		}

		FshDirEntry *dirs = reinterpret_cast<FshDirEntry*>(fshBytes + 16);

		int size = head->size;
		int nBmps = head->numBmps;

		int palIndex = -1;
		int palOffset = -1;
		bool hasGlobalPal = false;


		for (int i = 0; i < nBmps; i++)
		{
			if (!hasGlobalPal)
			{
				FshEntryHeader *hdr = reinterpret_cast<FshEntryHeader*>(fshBytes + dirs[i].offset);

				hasGlobalPal = (strncmp(dirs[i].name, "!pal", 4) == 0);
				int k = hdr->code & 255;
				if (hasGlobalPal && (k == 0x22 || k == 0x24 || k== 0x29 || k == 0x2a || k == 0x2d))
				{
					palIndex = i;
					palOffset = dirs[i].offset;
				}
			}
		}

		int nextOffset = size;

		for (int j = 0; j < nBmps; j++)
		{
			if (dirs[j].offset < nextOffset && dirs[j].offset > nextOffset)
			{
				nextOffset = dirs[j].offset;
			}
		}

		for (int i = 0; i < nBmps; i++)
		{
			FshEntryHeader* entry = reinterpret_cast<FshEntryHeader*>(fshBytes + dirs[i].offset); // only extract the first image

			bool compressed = (entry->code & 0x80) > 0;

			const int code = (entry->code & 0x7f);
			const int width = (int)entry->width;
			const int height = (int)entry->height;

			bool isImage = code == 0x7b || code == 0x7d || code == 0x7e || code == 0x7f || code == 0x78 || code == 0x6d || code == 0x60 || code == 0x61;

			if (!isImage)
			{
				continue;
			}

			FshEntryHeader* aux = entry;
			int auxOffset = dirs[i].offset;
			while ((aux->code >> 8) > 0)
			{
				auxOffset += (aux->code >> 8);

				if (auxOffset >= size)
				{
					break;
				}

				aux = reinterpret_cast<FshEntryHeader*>(fshBytes + auxOffset);

				if (code == 0x7b)
				{
					int paletteCode = aux->code & 255;
					if (paletteCode == 0x22 || paletteCode == 0x24 || paletteCode == 0x29 || paletteCode == 0x2a || paletteCode == 0x2d)
					{
						palOffset = auxOffset; // use the local palette if found
					}
				}
			}

			DWORD bmpStart = dirs[i].offset + 16;
			BYTE* bmpStartPtr = (fshBytes + bmpStart);
			BYTE* bmpDataPtr = nullptr;

			if (compressed)
			{
				DWORD compSize = 0;

				int sectionLength = entry->code >> 8;
				if (sectionLength > 0)
				{
					compSize = sectionLength;
				}
				else
				{
					compSize = nextOffset - bmpStart;
				}

				if ((bmpStartPtr[0] & 0xfe) != 0x10 && bmpStartPtr[1] == 0xfb)
				{
					return E_FAIL; // EaGraph and FshEd allow 16-bit and 32-bit data to be compressed with DXTn compression abort in that case.
				}

				hr = QFSDecompress(bmpStartPtr, &bmpDataPtr, compSize);

				if (FAILED(hr))
				{
					return hr;
				}
			}
			else
			{
				bmpDataPtr = bmpStartPtr;
			}

			BYTE* scan0 = nullptr;
			UINT imageSize = GetImageDataSize(width, height, code);

			if (code == 0x60 || code == 0x61)
			{
				scan0 = (BYTE*)LocalAlloc(LPTR, (SIZE_T)imageSize);
				if (!scan0)
					return E_OUTOFMEMORY;

				DecompressImage(scan0, width, height, bmpDataPtr, code == 0x60);
				hr = factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppRGBA, width * 4, imageSize, scan0, image);

				LocalFree(scan0);
			}
			else if (code == 0x7b)
			{
				hr = factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat8bppIndexed, width, imageSize, bmpDataPtr, image);
				if (SUCCEEDED(hr))
				{
					IWICPalette* palette = nullptr;
					hr = CreatePalette(palOffset, factory, &palette);

					if (SUCCEEDED(hr))
					{
						(*image)->SetPalette(palette);
						palette->Release();
					}

				}
			}
			else if (code == 0x7d) // 32-bit A8R8G8B8
			{
				hr = factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, width * 4, imageSize, bmpDataPtr, image);
			}
			else if (code == 0x7f) // 24-bit A0R8G8B8
			{
				hr = factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat24bppBGR, width * 3, imageSize, bmpDataPtr, image);
			}
			else if (code == 0x7e) // 16-bit A1R5G5B5
			{
				hr = factory->CreateBitmap(width, height, GUID_WICPixelFormat32bppBGRA, WICBitmapCacheOnDemand, image);

				if (SUCCEEDED(hr))
				{
					IWICBitmapLock* lock = nullptr;
					UINT32 stride;
					hr = LockBitmap(*image, &lock, &scan0, &stride);

					if (SUCCEEDED(hr))
					{
						USHORT* sPtr = (USHORT*)bmpDataPtr;
						for (int y = 0; y < height; y++)
						{
							USHORT* src = sPtr + (y * width);
							BYTE* dst = scan0 + (y * stride);

							for (int x = 0; x < width; x++)
							{
								dst[0] = (BYTE)((src[0] & 0x1f) << 3);
								dst[1] = (BYTE)(((src[0] >> 5) & 0x1f) << 3);
								dst[2] = (BYTE)(((src[0] >> 10) & 0x1f) << 3);
								if ((src[0] & 0x8000) > 0)
								{
									dst[3] = 255;
								}
								else
								{
									dst[3] = 0;
								}

								src++;
								dst += 4;
							}
						}

						lock->Release();
					}
				}
			}
			else if (code == 0x78)// 16-bit A0R5G5B5
			{
				hr = factory->CreateBitmap(width, height, GUID_WICPixelFormat24bppBGR, WICBitmapCacheOnDemand, image);

				if (SUCCEEDED(hr))
				{
					IWICBitmapLock* lock = nullptr;
					UINT32 stride;
					hr = LockBitmap(*image, &lock, &scan0, &stride);

					if (SUCCEEDED(hr))
					{
						USHORT* sPtr = (USHORT*)bmpDataPtr;
						for (int y = 0; y < height; y++)
						{
							USHORT* src = sPtr + (y * width);
							BYTE* dst = scan0 + (y * stride);

							for (int x = 0; x < width; x++)
							{
								dst[0] = (BYTE)((src[0] & 0x1f) << 3);
								dst[1] = (BYTE)(((src[0] >> 5) & 0x3f) << 2);
								dst[2] = (BYTE)(((src[0] >> 11) & 0x1f) << 3);

								src++;
								dst += 3;
							}
						}

						lock->Release();
					}
				}

			}
			else if (code == 0x6d) // 16-bit (4:4:4:4)
			{
				hr = factory->CreateBitmap(width, height, GUID_WICPixelFormat32bppBGRA, WICBitmapCacheOnDemand, image);

				if (SUCCEEDED(hr))
				{
					IWICBitmapLock* lock = nullptr;
					UINT32 stride;
					hr = LockBitmap(*image, &lock, &scan0, &stride);

					if (SUCCEEDED(hr))
					{
						int srcStride = width * 2;

						for (int y = 0; y < height; y++)
						{
							BYTE* src = bmpDataPtr + (y * srcStride);
							BYTE* dst = scan0 + (y * stride);

							for (int x = 0; x < width; x++)
							{

								dst[0] = (BYTE)((src[0] & 15) * 0x11);
								dst[1] = (BYTE)((src[0] >> 4) * 0x11);
								dst[2] = (BYTE)((src[1] & 15) * 0x11);
								dst[3] = (BYTE)((src[1] >> 4) * 0x11);

								src += 2;
								dst += 4;
							}
						}

						lock->Release();
					}
				}
			}

			if (compressed)
			{
				LocalFree(bmpDataPtr);
			}

			break; // we only want the first image in the file
		}
	}

	TraceLeaveHr(hr);

	return hr;
}

HRESULT ConvertBitmapSourceTo32BPPHBITMAP(IWICBitmapSource *pBitmapSource,
										  IWICImagingFactory *pImagingFactory,
										  HBITMAP *phbmp,
										  SIZE requestedSize)
{
	TraceEnter();
	*phbmp = nullptr;

	IWICBitmapSource *pBitmapSourceConverted = nullptr;
	WICPixelFormatGUID guidPixelFormatSource;
	HRESULT hr = pBitmapSource->GetPixelFormat(&guidPixelFormatSource);
	if (SUCCEEDED(hr) && (guidPixelFormatSource != GUID_WICPixelFormat32bppBGRA))
	{
		IWICFormatConverter *pFormatConverter;
		hr = pImagingFactory->CreateFormatConverter(&pFormatConverter);
		if (SUCCEEDED(hr))
		{
			// Create the appropriate pixel format converter
			hr = pFormatConverter->Initialize(pBitmapSource, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
			if (SUCCEEDED(hr))
			{
				hr = pFormatConverter->QueryInterface(&pBitmapSourceConverted);
			}
			pFormatConverter->Release();
		}
	}
	else
	{
		hr = pBitmapSource->QueryInterface(&pBitmapSourceConverted);  // No conversion necessary
	}

	IWICBitmapSource *pBitmapSourceScaled = nullptr;

	if (SUCCEEDED(hr))
	{

		UINT width, height;
		hr = pBitmapSourceConverted->GetSize(&width, &height);

		if (SUCCEEDED(hr))
		{
			if (requestedSize.cx > 0 && requestedSize.cy > 0)
			{
				UINT cx = min(requestedSize.cx, requestedSize.cy);
				SIZE size = ComputeThumbnailSize(width, height, cx);
				UINT newWidth = (UINT)size.cx;
				UINT newHeight = (UINT)size.cy;

				if (newWidth < width || newHeight < height)
				{
					IWICBitmapScaler *scaler = nullptr;
					hr = pImagingFactory->CreateBitmapScaler(&scaler);

					if (SUCCEEDED(hr))
					{
						hr = scaler->Initialize(pBitmapSourceConverted, newWidth, newHeight, WICBitmapInterpolationModeFant);
						if (SUCCEEDED(hr))
						{
							scaler->QueryInterface(&pBitmapSourceScaled);
						}
						scaler->Release();
					}
				}
				else
				{
					hr = pBitmapSourceConverted->QueryInterface(&pBitmapSourceScaled);
				}
			}
			else
			{
				hr = pBitmapSourceConverted->QueryInterface(&pBitmapSourceScaled);
			}
		}

		pBitmapSourceConverted->Release();
	}

	if (SUCCEEDED(hr))
	{

		UINT nWidth, nHeight;
		hr = pBitmapSourceScaled->GetSize(&nWidth, &nHeight);

		if (SUCCEEDED(hr))
		{
			BITMAPINFO bmi = {};
			bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth = nWidth;
			bmi.bmiHeader.biHeight = -static_cast<LONG>(nHeight);
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			BYTE *pBits;
			HBITMAP hbmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, reinterpret_cast<void **>(&pBits), nullptr, 0);
			hr = hbmp ? S_OK : E_OUTOFMEMORY;
			if (SUCCEEDED(hr))
			{
				WICRect rect = {0, 0, nWidth, nHeight};

				// Convert the pixels and store them in the HBITMAP.  Note: the name of the function is a little
				// misleading - we're not doing any extraneous copying here.  CopyPixels is actually converting the
				// image into the given buffer.
				hr = pBitmapSourceScaled->CopyPixels(&rect, nWidth * 4, nWidth * nHeight * 4, pBits);
				if (SUCCEEDED(hr))
				{
					*phbmp = hbmp;
				}
				else
				{
					DeleteObject(hbmp);
				}
			}
		}

		pBitmapSourceScaled->Release();
	}
	TraceLeaveHr(hr);

	return hr;
}


STDMETHODIMP CFshThumbnailHandler::Extract(HBITMAP *phBmpImage)
{
	HRESULT hr = S_OK;
	DWORD dwError = ERROR_SUCCESS;
	BOOL bResult = TRUE;
	TraceEnter();

	// Open file
	hFile = INVALID_HANDLE_VALUE;
	if (SUCCEEDED(hr))
	{
		LPCWSTR lpFileName = (LPCWSTR)m_bstrFileName;
		DWORD dwDesiredAccess = GENERIC_READ;
		DWORD dwShareMode = FILE_SHARE_READ;
		LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr;
		DWORD dwCreationDisposition = OPEN_EXISTING;
		DWORD dwFlagsAndAttributes = FILE_FLAG_SEQUENTIAL_SCAN;
		HANDLE hTemplateFile = nullptr;

		hFile = CreateFileW(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			lpSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			dwError = GetLastError();
			hr = HRESULT_FROM_WIN32(dwError);
			TraceOut("CreateFile failed, hr=0x%x", hr);
		}
	}

	if (SUCCEEDED(hr))
	{
		TraceOut("File open");
		IWICImagingFactory *pImagingFactory;
		hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pImagingFactory));

		if (SUCCEEDED(hr))
		{
			IWICBitmap *image = nullptr;

			hr = LoadFSH(pImagingFactory, &image);
			TraceOut("Loading fsh, hr = 0x%x", hr);

			if (SUCCEEDED(hr))
			{
				hr = ConvertBitmapSourceTo32BPPHBITMAP(image, pImagingFactory, phBmpImage, m_size);

				TraceOut("Creating thumbnail, hr = 0x%x", hr);


				image->Release();
				image = nullptr;
			}

			pImagingFactory->Release();
		}
	}


	// Cleanup
	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	if (nullptr != fshBytes)
	{
		LocalFree(fshBytes);
		fshBytes = nullptr;
	}

	TraceLeaveHr(hr);
	return hr;
}

STDMETHODIMP CFshThumbnailHandler::GetLocation(LPWSTR pszPathBuffer,
											 DWORD cchMax,
											 DWORD *pdwPriority,
											 const SIZE *prgSize,
											 DWORD dwRecClrDepth,
											 DWORD *pdwFlags)
{
	HRESULT hr = S_OK;
	TraceEnter();

	TraceOut("pszPathBuffer=%S", pszPathBuffer);
	TraceOut("cchMax=%u", cchMax);

	if (SUCCEEDED(hr))
	{
		if (nullptr == pszPathBuffer)
		{
			TraceOut("pszPathBuffer is nullptr");
			hr = E_INVALIDARG;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (nullptr == pdwPriority)
		{
			TraceOut("pdwPriority is nullptr");
			hr = E_INVALIDARG;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (nullptr == pdwFlags)
		{
			TraceOut("pdwFlags is nullptr");
			hr = E_INVALIDARG;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (nullptr == prgSize)
		{
			TraceOut("prgSize is nullptr");
			hr = E_INVALIDARG;
		}
	}

	TraceOut("*pdwFlags = %u", *pdwFlags);
	TraceOut("prgSize=%d x %d", prgSize->cx, prgSize->cy);

	if (SUCCEEDED(hr))
	{
		wcscpy_s(pszPathBuffer, cchMax, m_bstrFileName);

		*pdwPriority = IEIT_PRIORITY_NORMAL;

		if ((*pdwFlags & IEIFLAG_ASPECT) ||
			(*pdwFlags & IEIFLAG_ORIGSIZE))
		{
			m_size = *prgSize;
			TraceOut("m_size = %d x %d", m_size.cx, m_size.cy);
		}
		else
		{
			m_size.cx = -1;
			m_size.cy = -1;
		}

		*pdwFlags |= IEIFLAG_CACHE;

		if (*pdwFlags & IEIFLAG_ASYNC)
		{
			hr = E_PENDING;
		}
		else
		{
			hr = NOERROR;
		}
	}

	TraceLeaveHr(hr);
	return hr;
}

