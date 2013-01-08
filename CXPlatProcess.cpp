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
// Implementation of Process functions.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CXPlatProcess.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CXPlatProcess::CXPlatProcess()
{
    hProcess = 0;
}

CXPlatProcess::CXPlatProcess( char *szProcess, BOOL bHide, BOOL bWait, BOOL bInheritHandles )
{
    hProcess = 0;
    LaunchHiddenProcess( szProcess, bHide, bWait, bInheritHandles );
    return;
}

CXPlatProcess::~CXPlatProcess()
{ 
}

int CXPlatProcess::Launch( char *szProcess, BOOL bHide, BOOL bWait, BOOL bInheritHandles )
{
    return LaunchHiddenProcess( szProcess, bHide, bWait, bInheritHandles );
}

XPLAT_HANDLE CXPlatProcess::GetProcessHandle( void )
{
    return hProcess;
}

		// Launch a secondary process
int CXPlatProcess::LaunchHiddenProcess(char *szProcess, BOOL bHide, BOOL bWait, BOOL bInheritHandles)
{
#if defined(_WIN32)
    STARTUPINFO             sSUInfo;
    PROCESS_INFORMATION     sPI;
	MSG						sMsg;
 
    	// Create the process
	memset(&sSUInfo, 0, sizeof(sSUInfo));
	sSUInfo.cb = sizeof(sSUInfo);
	memset(&sPI, 0, sizeof(sPI));

        // If hidden . . .
    if (bHide)
        {
        sSUInfo.dwFlags = STARTF_USESHOWWINDOW;
        sSUInfo.wShowWindow = SW_HIDE; 
        };

        // If a 16 bit program, load in our own VDM and wait for the VDM to terminate
    if (!::CreateProcess(   NULL,
				            szProcess,
							NULL,
							NULL,
				            bInheritHandles,
				            CREATE_SEPARATE_WOW_VDM | CREATE_NEW_CONSOLE,
				            NULL,
				            NULL,
				            &sSUInfo,
				            &sPI))
        {
        return(-1);
        };

        // If it's a 16 bit, we have to process messages or we get deadlock
    while(bWait && ::WaitForSingleObject(sPI.hProcess, 1) == WAIT_TIMEOUT)
        {
	    while (::PeekMessage(&sMsg, NULL, 0, 0, PM_REMOVE))
		    {
            ::TranslateMessage(&sMsg);
            ::DispatchMessage(&sMsg);
		    }
        };

    hProcess = sPI.hProcess;

	return(sPI.dwProcessId);

#else

    // unix
        char        szCommand[1024];
        int         tPID = 0;


        //execute start script

        strncpy( szCommand, szProcess, sizeof(szCommand)-1 );

        #if defined(__linux)
        // NOTE: scheduling prty must be set
        setpriority(PRIO_PROCESS, 0, 0);
        #endif
        strcat( szCommand, " &" );
        tPID = system( szCommand );

       return tPID;

#endif
};


	
