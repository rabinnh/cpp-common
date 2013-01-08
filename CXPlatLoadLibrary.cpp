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
// Implementation of DLL/shared library functions.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CXPlatLoadLibrary.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CXPlatLoadLibrary::CXPlatLoadLibrary()
{
    hLib = NULL;
}

CXPlatLoadLibrary::CXPlatLoadLibrary( LPCSTR lib )
{
    hLib = NULL;
    LoadLibrary( lib );
}

CXPlatLoadLibrary::~CXPlatLoadLibrary()
{ 
    FreeLibrary();
}


XPLAT_HMODULE   CXPlatLoadLibrary::GetLib( void )
{
    return hLib;
}

XPLAT_HMODULE   CXPlatLoadLibrary::LoadLibrary( LPCSTR lib )
{
    if ( hLib ) 
        FreeLibrary();

    #if defined(_WIN32)
        
    hLib = ::LoadLibrary( lib );

    #else
    // unix

    #if defined(_AIX)
    hLib = dlopen( lib, RTLD_LAZY );
    #else
    // Solaris and Linux
    hLib = dlopen( lib, RTLD_LAZY );
    #endif
    
    #endif
    
    return hLib;
}

FARPROC         CXPlatLoadLibrary::GetProcAddress( LPCSTR proc )
{
    #if defined(_WIN32)	
    return ::GetProcAddress( hLib, proc );
    #else
    return ::dlsym( hLib, proc );
    #endif
}

FARPROC         CXPlatLoadLibrary::GetSymAddress( LPCSTR sym )
{
    #if defined(_WIN32)	
    return ::GetProcAddress( hLib, sym );
    #else
    return ::dlsym( hLib, sym );
    #endif
}

void            CXPlatLoadLibrary::FreeLibrary(void)
{
    #if defined(_WIN32)
	if (hLib)
		::FreeLibrary(hLib);
    #else	
	if (hLib)
            dlclose(hLib);
    #endif
    hLib = NULL;
}

	
