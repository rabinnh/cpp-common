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
*/// CXPlatThread.cpp: implementation of the CXPlatThread class.
//
//
//	Created: Richard A. Bross
//
//////////////////////////////////////////////////////////////////////
// NOTES:
//		On Unix platforms, every thread must be either detached or
//		WaitForThreadCompletion(INFINITE) must be called for proper
//		cleanup.  If WaitForThreadCompletion is called with any timeout
//		other than INFINITE, and it times out, you must then call
//		either DetachThread or WaitForThreadCompletion(INFINITE)
//		if you wish to ensure proper cleanup prior to detruction of
//		the object..
//
//		However, if neither has been called, and the object is deleted
//		the destructor will automatically call
//		WaitForThreadCompletion(INFINITE).
//


#if defined(_WIN32)
#include "stdafx.h"
#endif
#include "XPlat.h"
#include "CXPlatThread.h"
#include <iostream>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXPlatThread::CXPlatThread()
{
    memset(&hThreadHandle, 0, sizeof (hThreadHandle));
    bSuspended = FALSE;
    pUserParm = NULL;
#if !defined(_WIN32)
    bDetached = bJoined = FALSE;
#endif
};

CXPlatThread::~CXPlatThread()
{
    if (hThreadHandle)
        CloseThreadHandle();
};

// Start thread execution

CXPlatThread::THREAD_STATUS CXPlatThread::StartThread(void *pParam, BOOL bStartSuspended)
{
    int iStartFlag;
    THREAD_STATUS eStatus = GetThreadStatus();

    // Check to see if thread is active
    if (eStatus != NOT_STARTED && eStatus != THREAD_TERMINATED)
        return (eStatus);

    // If handle exists, thread already ran at least once
    if (hThreadHandle)
    {
        if (IsThreadActive(hThreadHandle))
            return (ALREADY_RUNNING);
        CloseThreadHandle();
    };

    pUserParm = pParam;

    // Start thread
    iStartFlag = bStartSuspended ? 0 : 1;
#if defined(_WIN32)
    unsigned uThread;

    // If you are using Visual C++, and you get an error here,
    // make sure under Project|Settings|C++|Code Generation
    // that you are linking with the Multithreaded libraries.
    hThreadHandle = (HANDLE) ::_beginthreadex(NULL,
                                              0,
                                              ThreadFunc,
                                              (void *) this,
                                              iStartFlag,
                                              &uThread);

    // Save whether thread is suspended
    if (hThreadHandle)
        bSuspended = bStartSuspended;
#else
    iStartFlag = ::pthread_create(&hThreadHandle,
                                  pthread_attr_default,
                                  (START_ROUTINE) ThreadFunc,
                                  (void *) this);
#endif
    // Return start status
    return (!hThreadHandle ? CANT_START : THREAD_SUCCESS);
};


// Suspend thread

CXPlatThread::THREAD_STATUS CXPlatThread::SuspendThread()
{
    THREAD_STATUS eStatus = GetThreadStatus();

#if !defined(_WIN32)
    return (NOT_SUPPORTED_ON_PLATFORM);
#endif
    // Suspend?
    if (eStatus != RUNNING)
        return (eStatus == SUSPENDED ? ALREADY_SUSPENDED : eStatus);

    // Suspend
#if defined(_WIN32)
    if (::SuspendThread(hThreadHandle) != -1)
        bSuspended = TRUE;

    return (SUSPENDED);
#else
    return (NOT_SUPPORTED_ON_PLATFORM);
#endif
};


// Resume thread

CXPlatThread::THREAD_STATUS CXPlatThread::ResumeThread()
{
#if !defined(_WIN32)
    return (NOT_SUPPORTED_ON_PLATFORM);
#endif

#if defined(_WIN32)
    THREAD_STATUS eStatus = GetThreadStatus();

    // Resume?
    if (eStatus != SUSPENDED)
        return (RESUMED);

    if (::ResumeThread(hThreadHandle) != -1)
        bSuspended = FALSE;

    return (RESUMED);
#endif
};


// Return the thread handle

XPLAT_HANDLE CXPlatThread::GetThreadHandle()
{
    return (hThreadHandle);
};

// Thread function

unsigned CXPlatThread::ThreadFunc(void *pParam)
{
    CXPlatThread *cThread;
    unsigned uReturn;

    cThread = (CXPlatThread *) pParam;

    uReturn = cThread->WorkerFunction(cThread->pUserParm);

    return (uReturn);
};



// Check to see if thread is active
// Returns already running

CXPlatThread::THREAD_STATUS CXPlatThread::GetThreadStatus()
{
    // Not even allocated?
    if (!hThreadHandle)
        return (NOT_STARTED);

#if !defined(_WIN32)
    return (IsThreadActive(hThreadHandle) ? RUNNING : THREAD_TERMINATED);
#else
    DWORD dwTerminationStatus;

    // Will fail if not a valid handle
    if (!::GetExitCodeThread(hThreadHandle, (unsigned long *) &dwTerminationStatus))
        return (THREAD_TERMINATED);
    //Check termination status
    if (dwTerminationStatus == STILL_ACTIVE)
        return (bSuspended ? SUSPENDED : RUNNING);
    return (THREAD_TERMINATED);
#endif
};


// Wait for a thread to terminate.  After timeout, kill it with extreme prejudice.

DWORD CXPlatThread::WaitForThreadCompletion(DWORD dwTimeout)
{
#if defined(_WIN32)	
    if (!hThreadHandle)
        return 0;
    else
        return (::WaitForSingleObject(hThreadHandle, dwTimeout));
#else
    if (!hThreadHandle)
        return (0);
    if (bDetached)
        return (INVALID_REQUEST);
    if (bJoined)
        return (0);
    // Bogus kudge because pthread_join doesn't have a timeout
    if (dwTimeout != INFINITE)
    {
        int iMaxWait = dwTimeout;
        int iCurWait = 0;
        BOOL bTimeout = TRUE;

        do
        { // Wait for the thread to complete
            if (!IsThreadActive(hThreadHandle))
            { // Thread completed
                bTimeout = FALSE;
                break;
            };
            usleep(1); // Wait timeout for it to complete
        }
        while(++iCurWait < iMaxWait);
        if (bTimeout)
            ::pthread_cancel(hThreadHandle);
    }

    bJoined = TRUE;
    int iRet = 0;
    if (hThreadHandle)
    {
        iRet = ::pthread_join(hThreadHandle, NULL);
        ::memset(&hThreadHandle, 0, sizeof(hThreadHandle));
    }
    return (iRet);
#endif
};


// Return a user defined thread type ID

int CXPlatThread::GetThreadType()
{
    return (0);
};


// Close handle

void CXPlatThread::CloseThreadHandle()
{
#if defined(_WIN32)
    ::CloseHandle(hThreadHandle);
#else
    // Wait for thread to finish
    if (!bJoined && !bDetached)
        WaitForThreadCompletion(INFINITE);
#endif
    memset(&hThreadHandle, 0, sizeof (hThreadHandle));
    return;
};


// Is this thread active

BOOL CXPlatThread::IsThreadActive(XPLAT_HANDLE hThread)
{
#if defined(_WIN32)
    if (hThread)
    {
        if (::WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT)
            return (TRUE);
    };
#else
    int iReturn;
    if (!hThread)
        return (FALSE);
    // With a signal value of 0, it will just find the thread, not actually kill it
    iReturn = ::pthread_kill(hThread, 0);
    if (iReturn != ESRCH)
        return (TRUE);
#endif

    return (FALSE);
};


#if !defined(_WIN32)
// You must either detach the thread or join it for proper cleanup

int CXPlatThread::DetachThread()
{
    if (!hThreadHandle)
        return (THREAD_TERMINATED);
    if (bJoined)
        return (INVALID_REQUEST);
    if (!bDetached)
    {
        ::pthread_detach(hThreadHandle);
        bDetached = TRUE;
    };
    return (THREAD_SUCCESS);
};
#endif

