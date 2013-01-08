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
#if defined(_WIN32)
#include "stdafx.h"
#endif
#include "XPlat.h"
#include "CXPlatCriticalSection.h"
#include "CXPlatEvent.h"

#if !defined(_WIN32)
// To protect calls that are not threadsafe
CXPlatCriticalSection cProtectCall;

// Convert a string to uppercase

char *_strupr(char *string1)
{
    char *szWork;

    // If not NULL
    if ((szWork = string1) != NULL)
    {
        while(*szWork)
        {
            if (*szWork >= 'a' && *szWork <= 'z')
                *szWork += ('A' - 'a');
            szWork++;
        };
    };
    return (string1);
};




// Convert a string to lowercase

char *_strlwr(char *string1)
{
    char *szWork;

    // If not NULL
    if ((szWork = string1) != NULL)
    {
        while(*szWork)
        {
            if (*szWork >= 'A' && *szWork <= 'Z')
                *szWork -= ('A' - 'a');
            szWork++;
        };
    };
    return (string1);
};



// Itoa

char *itoa(int value, char *szString, int radix)
{
    char *szFormat;

    switch(radix) {
        case 16:
            szFormat = (char *) "0x%x";
        case 10:
        default:
            szFormat = (char *) "%d";
            break;
    };

    ::sprintf(szString, szFormat, value);

    return (szString);
};

#endif


// XPlat millisecond timer

unsigned long XPlatGetMilliseconds()
{
#if !defined(_WIN32)
    struct timeval tv;
    struct timezone tz;
    unsigned int iMS;

    ::gettimeofday(&tv, NULL);
    iMS = tv.tv_sec * 1000;
    iMS += (tv.tv_usec / 1000);
    return (iMS);
#else
    return (::GetTickCount());
#endif
};


// Execute process

int XPlatForkHiddenProcess(const char *szProcess)
{
#if defined(_WIN32)
    STARTUPINFO sSUInfo;
    PROCESS_INFORMATION sPI;

    // Create the process
    memset(&sSUInfo, 0, sizeof (sSUInfo));
    sSUInfo.cb = sizeof (sSUInfo);
    memset(&sPI, 0, sizeof (sPI));

    // If hidden . . .
    sSUInfo.dwFlags = STARTF_USESHOWWINDOW;
    sSUInfo.wShowWindow = SW_HIDE;

    // If a 16 bit program, load in our own VDM and wait for the VDM to terminate
    if (!::CreateProcess(NULL,
                         (char *) szProcess,
                         NULL,
                         NULL,
                         TRUE,
                         CREATE_SEPARATE_WOW_VDM | CREATE_NEW_CONSOLE,
                         NULL,
                         NULL,
                         &sSUInfo,
                         &sPI))
    {
        return (-1);
    };

    return (sPI.dwProcessId);
#else

    /*#if defined(__linux)
        int		tPID;
        tPID = ::fork();	// fork a new process
        if (tPID == 0)
            {	// NOTE: the bash shell (sh) cannot be used,
                // because of suid priv. and the scheduling prty must be set
            ::setpriority(PRIO_PROCESS, 0, 0);
            ::execl( "/bin/tcsh", "/bin/tcsh", "-c", szProcess, 0);
            ::exit(127);	// We ran our new program, so exit this process
            }
    #else */
    string sCommand = szProcess;
    sCommand += " &";
    return ::system(sCommand.c_str());
    //#endif
#endif	// WIN32

};


#if !defined(_WIN32)
// Given the time to wait in ms get the abstime

void GetAbstime(struct timespec &sTimespec, unsigned long lTimeToWaitMS)
{
    struct timeval now;
    long lnSec;

    memset(&sTimespec, 0, sizeof(sTimespec));
    // timeout for pthread_cond_timedwait is the absolute time.  dwWait is in ms.
    // gettimeofday uses timeval, which is seconds and microseconds, we need a timespec which is seconds and nanoseconds
    gettimeofday(&now, NULL);
    sTimespec.tv_sec = now.tv_sec + (lTimeToWaitMS / 1000);
    lnSec = now.tv_usec * 1000; // Convert microseconds to nanoseconds
    sTimespec.tv_nsec = lnSec + ((lTimeToWaitMS % 1000) * 1000000); // We now have our nano seconds (ms * 1000000)
    // If nano seconds > a billion, roll over
    while(sTimespec.tv_nsec > 1000000000)
    {
        sTimespec.tv_nsec -= 1000000000;
        sTimespec.tv_sec++;
    }
}
#endif




