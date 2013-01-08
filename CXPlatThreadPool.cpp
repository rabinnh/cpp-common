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
*///! CXPlatThreadPool.cpp: implementation of the CXPlatThreadPool class.
//
//
//	Created: Richard A. Bross
//
//
//! This class manages a pool of CXPlatPooledThread objects
//
#include "XPlat.h"
#include "CXPlatEvent.h"
#include "CXPlatPooledThread.h"
#include "CXPlatThreadPool.h"
#include <iostream>

// Construct the thread pool

CXPlatThreadPool::CXPlatThreadPool(int iPoolSize, int iMaxSize, int iGrowBy)
{
    if (iPoolSize < 1)
        iPoolSize = 1;
    iOriginalThreadsInPool = iPoolSize;
    if (iMaxSize < iPoolSize)
        iMaxSize = iPoolSize;
    iMaxThreadsAllowed = iMaxSize;
    if (iGrowBy < 1)
        iGrowBy = 1;
    this->iGrowBy = iGrowBy;
    bShrinking = false;
    // This will only get used if explicitly set prior to thread creation
    lWaitToFinish = INFINITE;
    bWaitToFinishSet = false;
};


// Destructor

CXPlatThreadPool::~CXPlatThreadPool()
{
    int iIndex;
    int iSize;
    CXPlatPooledThread *pThread;

    // Delete threads.
    // Thread destructors take care of details.
    cPoolCritSect.Lock();
    iSize = cThreadPool.size();
    for (iIndex = 0; iIndex < iSize; iIndex++)
    {
        pThread = cThreadPool[iIndex];
        if (pThread)
            delete pThread;
    };
    cThreadPool.clear();
    cPoolCritSect.Unlock();
};

BOOL CXPlatThreadPool::Init()
{
    cPoolCritSect.Lock();
    GrowThreadPool(TRUE);
    cPoolCritSect.Unlock();
    return (TRUE);
};


// Add threads to the pool

BOOL CXPlatThreadPool::GrowThreadPool(BOOL bInit)
{
    int iIndex;
    CXPlatPooledThread *pThread;
    int iGrowTo;
    BOOL bSuccess = FALSE;

    // If we're initializing, grow to the original size,
    // else, grow by the amount that's been set up to the max.
    iGrowTo = bInit ? iOriginalThreadsInPool : cThreadPool.size() + iGrowBy;
    if (iGrowTo > iMaxThreadsAllowed)
        iGrowTo = iMaxThreadsAllowed;
    for (iIndex = cThreadPool.size(); iIndex < iGrowTo; iIndex++)
    {
        pThread = AllocatePooledThread();
        if (!pThread)
        {
            bSuccess = FALSE;
            break;
        };
        bSuccess = TRUE;
        pThread->pPoolParent = this;
        if (bWaitToFinishSet)
            pThread->SetCompletionWaitTime(lWaitToFinish);
        cThreadPool.push_back(pThread);
    };

    return (bSuccess);
};


// Shrink the thread pool by one thread
void CXPlatThreadPool::ShrinkPool()
{   // Don't do it if we already have a shrink request pending
    if (iMaxThreadsAllowed < cThreadPool.size())
        return;
    if (--iMaxThreadsAllowed == 0)
        iMaxThreadsAllowed = 1;
}


// Array will be set with available thread objects.
//
// If the return is > 0, it's the number of threads returned.
// A return of < 0 will be one of the error enums.
//
// If bWaitForAtLeastOne == TRUE, at least one thread must
// become available or the call will wait
// dwWaitTime ms before returning.
//
// Threads returned will be marked as allocated until
// the CXPlatPooledThread::PooledWorkerFunction returns.
// If CXPlatPooledThread::StartWorking is never called,
// this thread will never become available again.
//

int CXPlatThreadPool::GetAvailableThreads(THREAD_POOL_ARRAY &cAvailThreadPool, int iNeeded, BOOL bWaitForAtLeastOne, DWORD dwWaitTime)
{
    int iIndex;
    int iSize;
    CXPlatPooledThread *pThread;
    BOOL bSuccess = FALSE;
    int iReturn = 0;

    // Clear the passed in array
    cAvailThreadPool.clear();

    // Make sure the call is valid
    if (!iNeeded)
        return (TP_ZERO_THREADS_REQUESTED);

    // Make sure there are enough.
    // If even one won't do and they are asking
    // for more than are allowed, it's an error.
    if (!bWaitForAtLeastOne && iNeeded > iMaxThreadsAllowed)
        return (TP_REQUEST_EXCEEDS_POOL_SIZE);

    // Lock access to the thread pool
    cPoolCritSect.Lock();

    // Meet the demand
    do
    {   
        bool bShrunk = false;
        
        // Clear the passed in array to start over.
        cAvailThreadPool.clear();
        iSize = cThreadPool.size();
        // Load their array with the available thread objects
        for (iIndex = 0; iIndex < iSize; iIndex++)
        {
            pThread = cThreadPool[iIndex];
            if (!pThread)
                continue; // How could this ever happen? Defensive coding.

            // We don't have to worry about a race condition with
            // bThreadAllocated because it's only checked and set TRUE here,
            // and we're protected by the critical section.
            if (pThread->bThreadAllocated)
                continue;
            
            // See if we need to shrink the pool
            if (iSize > iMaxThreadsAllowed) 
            {
                delete pThread;
                cThreadPool.erase(cThreadPool.begin() + iIndex);
                bShrunk = true;
                break;
            }
            
            cAvailThreadPool.push_back(pThread);
            if (cAvailThreadPool.size() == iNeeded)
                break;
        };
        
        // If we shrunk the pool, try again
        if (bShrunk)
            continue;

        // If we've gotten all we needed and there was no shrinkage, cool.
        if (cAvailThreadPool.size() == iNeeded)
        {
            bSuccess = TRUE;
            break;
        }

        // We didn't get all the threads we needed.
        // First, see if there's room to grow
        if (GrowThreadPool())
            continue; // We grew the thread pool, so try again.

        // Did they want to wait for at least one?
        if (!bWaitForAtLeastOne) // They wanted all or none.
        {
            iReturn = TP_NOT_ENOUGH_THREADS_AVAILABLE;
            break;
        };

        // Couldn't grow the pool.  Is at least one sufficient?
        if (cAvailThreadPool.size())
        {
            bSuccess = TRUE;
            break;
        };

        // Wait for a thread to become available.
        // In *nix systems you have to be careful.  Unlike Windows, when an object is set before 
        // pthread_cond_wait, it does not recognize that it is signaled.  This is an issue with multiple
        // threads setting the event.  So in that rare case, check every 20 ms just to be sure
        iReturn = cThreadAvailableEvent.Lock(dwWaitTime);
        if (iReturn == XPLAT_TIMEOUT)
        {
            iReturn = TP_WAIT_TIMED_OUT;
            break;
        };
        // At least one came free.  Start over.
    }
    while (!bSuccess);

    // Success?  If not, clear the pool.
    if (!bSuccess)
        cAvailThreadPool.clear();
    else
    { // Success! Mark stored threads as allocated.
        iReturn = cAvailThreadPool.size();
        for (iIndex = 0; iIndex < iReturn; iIndex++)
        {
            pThread = cAvailThreadPool[iIndex];
            if (pThread)
                pThread->bThreadAllocated = TRUE;
        };
    };

    // Now it's safe to unlock the critical section
    cPoolCritSect.Unlock();

    return (iReturn);
};


// How many threads are currently idle (and therefore available)
// Note that you cannot depend on these threads remaining available if there is 
// more than one thread of execution in your program that dispatches the threads.

int CXPlatThreadPool::QueryThreadsAvailable()
{
    int iIndex;
    int iSize;
    CXPlatPooledThread *pThread;
    int iCount = 0;

    // Lock access to the thread pool
    cPoolCritSect.Lock();
    iSize = cThreadPool.size();
    // Load their array with the available thread objects
    for (iIndex = 0; iIndex < iSize; iIndex++)
    {
        pThread = cThreadPool[iIndex];
        if (!pThread)
            continue; // How could this ever happen? Defensive coding.
        if (!pThread->bThreadAllocated)
            iCount++;
    };

    cPoolCritSect.Unlock();

    iCount += (iMaxThreadsAllowed - iSize);

    return (iCount);
};


// Get how many threads are in the pool

int CXPlatThreadPool::GetCurrentSizeOfPool()
{
    int iSize;

    // Lock access to the thread pool
    cPoolCritSect.Lock();
    iSize = cThreadPool.size();
    cPoolCritSect.Unlock();

    return (iSize);
};


// Get the maximum size the thread pool can grow to.

int CXPlatThreadPool::GetMaxSize()
{
    return(iMaxThreadsAllowed);
};

// Set the maximum size the thread pool can grow to.

void CXPlatThreadPool::SetMaxSize(int iMaxSize)
{
    iMaxThreadsAllowed = iMaxSize;
};

// Wait for all threads to finish
void CXPlatThreadPool::WaitForAllThreadsToFinish()
{   
    // Wait until all active threads in the pool are available
    // QueryThreadsAvailable actually includes threads not yet allocated
    while (QueryThreadsAvailable() != iMaxThreadsAllowed)
        cThreadAvailableEvent.Lock();
}

// Set the wait time when we are waiting for all threads to finish
void CXPlatThreadPool::SetFinishWaitTime(unsigned long lWaitTime)
{
    lWaitToFinish = lWaitTime;
    bWaitToFinishSet = true;
}
