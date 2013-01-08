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
// CXPlatPooledThread.cpp: implementation for the CXPlatPooledThread class.

#include "CProfileTimer.h"

//
//
//	Created: Richard A. Bross
//
//////////////////////////////////////////////////////////////////////
//		These threads are used and managed by the CThreadPool
//      class.  You should derive a class from this class
//      and implement a PooledWorkerFunction to do your work.
//
//      The PooledWorkerFunction should do a job and return,
//      allowing this thread to be reused.
//
//      The thread is created on construction, but work will
//      not begin until you call "StartWorking".  There is
//      not an "EndWorking" call, because if the thread is
//      busy, it is in the user's code.  The thread is
//      destroyed on object destruction.
//
//      You can check the IsThreadBusy method to see if
//      the thread is busy or if it is available for work.
//
//      If a CXPlatEvent has been passed into
//      the constructor, the thread will set it when it is
//      available for work.

#include "XPlat.h"
#include "CXPlatEvent.h"
#include "CXPlatThread.h"
#include "CXPlatPooledThread.h"
#include "CXPlatThreadPool.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXPlatPooledThread::CXPlatPooledThread() : CXPlatThread()
{
    bThreadBusy = bTerminateThread = bThreadAllocated = false;
    lWaitForCompletion = INFINITE;
    StartThread(this);
};

CXPlatPooledThread::~CXPlatPooledThread()
{
    bTerminateThread = TRUE;
    StartWorking();
    WaitForThreadCompletion(lWaitForCompletion); 
};


// Start doing the user's duty

int CXPlatPooledThread::StartWorking()
{
    cStartWorking.SetEvent();
    return(0);
};


// Is thread in the user's code?

BOOL CXPlatPooledThread::IsThreadBusy()
{
    return(bThreadBusy);
};

// Implemented by this class. SHOULD NOT BE DERIVED FROM!
// Use PooledWorkerFunction instead.

unsigned CXPlatPooledThread::WorkerFunction(void *pParam)
{
    pFuncParam = pParam;
    unsigned uReturn = 0;
    do
    {   // Wait for the "Start working" signal
        cStartWorking.Lock();
        // Check the terminate flag
        if (bTerminateThread)
            return(uReturn);
        // We're busy
        bThreadBusy = TRUE;

        // Call the user's function
        uReturn = PooledWorkerFunction();
        // They're done.  We're no longer busy.
        bThreadBusy = bThreadAllocated = FALSE;
        // Notify the thread pool that we are once again available.
        if (pPoolParent)
            pPoolParent->cThreadAvailableEvent.SetEvent();
    }
    while(!bTerminateThread);

    return(uReturn);
};


// Set the max time we will wait for thread to finish work when we are ready to terminate it
void CXPlatPooledThread::SetCompletionWaitTime(unsigned long lWaitTime)
{
    lWaitForCompletion = lWaitTime;
}
