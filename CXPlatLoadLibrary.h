/*
   Copyright [2010] [Richard Bross]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
//
//! Cross platform class to load DLL/shared library
//

#if !defined(AFX_CXPLATLOADLIBRARY_H__C80CD926_F97A_11D2_823E_00A0CC20AAD9__INCLUDED_)
#define AFX_CXPLATLOADLIBRARY_H__C80CD926_F97A_11D2_823E_00A0CC20AAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "XPlat.h"

#if !defined(_WIN32)
#if defined(__hpux)
#include <dl.h>
#else
#include <dlfcn.h>
#endif
#endif

//! Load a DLL/shared library
class CXPlatLoadLibrary  
{
public:

	CXPlatLoadLibrary();
	CXPlatLoadLibrary( LPCSTR );
	virtual         ~CXPlatLoadLibrary();

    XPLAT_HMODULE   GetLib();
    XPLAT_HMODULE   LoadLibrary( LPCSTR );
    FARPROC         GetProcAddress( LPCSTR );
    FARPROC         GetSymAddress( LPCSTR );
    void            FreeLibrary();

protected:
			
	XPLAT_HMODULE			   	hLib;
};

#endif //! !defined(AFX_CXPLATLOADLIBRARY_H__C80CD926_F97A_11D2_823E_00A0CC20AAD9__INCLUDED_)
