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
*///! CXPlatThreadPool.h: interface for the CXPlatThreadPool class.
//
//
//!	Created: Richard A. Bross
//
//! This class manages a pool of CXPlatPooledThread objects
//
#if !defined(CXPlatThreadPool_H__INCLUDED_)
#define CXPlatThreadPool_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif //! _MSC_VER >= 1000

#include "XPlat.h"
#include "CXPlatCriticalSection.h"
#include "CXPlatPooledThread.h"

// Array of threads in the pool
typedef vector<CXPlatPooledThread *> THREAD_POOL_ARRAY;

//! Implementation of thread pool class
class CXPlatThreadPool
{
    friend class CXPlatPooledThread;
public:
    //! Initial poolsize, maximum size to grow to, size to grow by when pool is grown
    CXPlatThreadPool(int iPoolSize = 5, int iMaxSize = 0, int iGrowBy = 1);
    virtual ~CXPlatThreadPool();

    // Error enums
    enum {
        TP_REQUEST_EXCEEDS_POOL_SIZE = -1,
        TP_NOT_ENOUGH_THREADS_AVAILABLE = -2,
        TP_ZERO_THREADS_REQUESTED = -3,
        TP_NO_THREADS_IN_POOL = -4,
        TP_WAIT_TIMED_OUT = -5
    };

    //! Array will be set with available thread objects.
    //!
    //! If the return is > 0, it's the number of threads returned.
    //! A return of < 0 will be one of the above error enums.
    //!
    //! If bWaitForAtLeastOne == TRUE, at least one thread must
    //! become available or the call will wait
    //! dwWaitTime ms before returning.
    //!
    //! Threads returned will be marked as allocated until
    //! the CXPlatPooledThread::PooledWorkerFunction returns.
    //! If CXPlatPooledThread::StartWorking is never called,
    //! this thread will never become available again.
    //!
    int GetAvailableThreads(THREAD_POOL_ARRAY &cAvailThreadPool, int iNeeded, BOOL bWaitForAtLeastOne = TRUE, DWORD dwWaitTime = INFINITE);
    //! How many threads are currently unallocated by requests?
    //! Note that you cannot depend on these threads remaining available
    //! if there is more than one than one thread of execution
    //! in your program that dispatches the threads.
    int QueryThreadsAvailable();
    //! Get how many threads are in the pool
    int GetCurrentSizeOfPool();
    //! Get the maximum size the thread pool can grow to.
    int GetMaxSize();
    //! Set the maximum size the thread pool can grow to.
    void SetMaxSize(int iMaxSize);
    //! Wait for all threads to finish.  Do NOT call GetAvailableThreads at the same time.
    void WaitForAllThreadsToFinish();
    //! Shrink the thread pool by one thread
    void ShrinkPool();
    //! Set the wait time when we are waiting for all threads to finish
    void SetFinishWaitTime(unsigned long lWaitTime);
protected:
    //! Implement to allocate the proper type of derived pooled thread
    virtual CXPlatPooledThread *AllocatePooledThread() = 0;
    //! Add threads to the pool
    BOOL GrowThreadPool(BOOL bInit = FALSE);
    //! Derived classes should call this to initial the thread pool
    //! Can be called in the constructor
    BOOL Init();

protected:
    THREAD_POOL_ARRAY cThreadPool; //! Our pool of threads
    CXPlatCriticalSection cPoolCritSect; //! To protect ThreadPool
    CXPlatEvent cThreadAvailableEvent; //! Shared event, a thread will reset when it becomes available
    //! Threads will set when they become available.
    int iOriginalThreadsInPool;
    int iThreadsInPool;
    int iMaxThreadsAllowed;
    int iGrowBy;
    bool bShrinking;
    unsigned long lWaitToFinish;
    // If not explicity set, lWaitToFinish will not be used and we will used the derived thread's default.
    bool bWaitToFinishSet;
};

#endif
