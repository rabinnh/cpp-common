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
//!
//! Launch a separate process
//!
//

#if !defined(CXPLATPROCESS_H)
#define CXPLATPROCESS_H

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "XPlat.h"

//! Class to launch a process
class CXPlatProcess  
{
public:

	CXPlatProcess();
	CXPlatProcess( char *szProcess, BOOL bHide = TRUE, BOOL bWait = FALSE, BOOL bInheritHandles = TRUE );

	virtual         ~CXPlatProcess();

    //! Launch and return process ID (NT)
	int Launch( char *szProcess, BOOL bHide = TRUE, BOOL bWait = FALSE, BOOL bInheritHandles = TRUE );

    XPLAT_HANDLE CXPlatProcess::GetProcessHandle();

protected:
	//! Launch a process hidden (this is mostly for Windows)
	int		LaunchHiddenProcess(char *szProcess, BOOL bHide = TRUE, BOOL bWait = FALSE, BOOL bInheritHandles = TRUE);
    XPLAT_HANDLE          hProcess;
};

#endif 
