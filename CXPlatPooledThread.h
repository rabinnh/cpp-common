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
//! CXPlatPooledThread.h: interface for the CXPlatPooledThread class.
//
//
//!	Created: Richard A. Bross
//
//!
//!	 These threads are used and managed by the CThreadPool
//!      class.  You should derive a class from this class
//!      and implement a PooledWorkerFunction to do your work.
//!
//!      The PooledWorkerFunction should do a job and return,
//!      allowing this thread to be reused.
//!
//!      The thread is created on construction, but work will 
//!      not begin until you call "StartWorking".  There is
//!      not a "EndWorking" call, because if the thread is
//!      busy, it is in the user's code.  Thre thread is
//!      destroyed on object destruction.
//!
//!      You can check the IsThreadBusy method to see if
//!      the thread is busy or if it is available for work.
//!
//!      If a CXPlatEvent has been passed into
//!      the constructor, the thread will set it when it is
//!      available for work.
//
#if !defined(CXPlatPooledThread_H__INCLUDED_)
#define CXPlatPooledThread_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif //! _MSC_VER >= 1000

#include "CXPlatThread.h"
#include "CXPlatEvent.h"

class CXPlatThreadPool;
class CXPlatEvent;

class CXPlatPooledThread : public CXPlatThread
{   //! So it can set the bThreadAllocated flag
    friend class CXPlatThreadPool; 
public:
    CXPlatPooledThread();
    virtual ~CXPlatPooledThread();

    //! Start doing the user's duty.
    //! **** NEVER CALL THIS TWICE! ****
    //! After your PooledWorkerFunction returns,
    //! this thread is added back to the thread pool!
    virtual int StartWorking();
    //! Is thread in the user's code?
    virtual BOOL IsThreadBusy();
    //! Set the max time we will wait for thread to finish work when we are ready to terminate it
    void SetCompletionWaitTime(unsigned long lWaitTime);
protected:
    //! Implemented by this class. SHOULD NOT BE DERIVED FROM!
    //! Use PooledWorkerFunction instead.
    unsigned WorkerFunction(void *pParam);
    //! This is what you should derive from to do your work.
    //! Note that once PooledWorkerFunction returns,
    //! this thread is returned to the thread pool and
    //! you must request a new thread for any further work.
    virtual unsigned PooledWorkerFunction() = 0;

protected:
    CXPlatThreadPool *pPoolParent;  //! Pool that we belong to
    CXPlatEvent cStartWorking;      //! When set, work starts
    BOOL bThreadBusy;
    BOOL bTerminateThread;
    BOOL bThreadAllocated;
    void *pFuncParam;
    unsigned long lWaitForCompletion;
};

#endif
