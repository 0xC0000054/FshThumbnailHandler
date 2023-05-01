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

#include <windows.h>

class CClassFactory 
    : public IClassFactory
{
public:
    CClassFactory (CLSID clsid);
    ~CClassFactory ();

    // IUnknown methods
    STDMETHODIMP QueryInterface (REFIID riid, void **ppvObject);
    STDMETHODIMP_(DWORD) AddRef ();
    STDMETHODIMP_(DWORD) Release ();

    // IClassFactory methods
    STDMETHODIMP CreateInstance (IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    STDMETHODIMP LockServer (BOOL fLock);

    void Constructor (CLSID ClassID);
    void Destructor (void);

protected:
    LONG m_lRefCount;

private:
    CLSID m_clsidObject;
};

