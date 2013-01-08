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
*///! CXPlatThread.h: interface for the CXPlatThread class.
//
//
//!	Created: Richard A. Bross
//
//
//! NOTES:
//!		On Unix platforms, every thread must be either detached or
//!		WaitForThreadCompletion(INFINITE) must be called for proper
//!		cleanup.  If WaitForThreadCompletion is called with any timeout
//!		other than INFINITE, and it times out, you must then call 
//!		either DetachThread or WaitForThreadCompletion(INFINITE)
//!		if you wish to ensure proper cleanup prior to detruction of
//!		the object..
//!
//!		However, if neither has been called, and the object is deleted
//!		the destructor will automatically call 
//!		WaitForThreadCompletion(INFINITE).
//
#if !defined(CXPlatThread_H__INCLUDED_)
#define CXPlatThread_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif //! _MSC_VER >= 1000

#include "XPlat.h"

class CXPlatThread {
public:
    CXPlatThread();
    virtual ~CXPlatThread();

    //! Current status of threads

    enum THREAD_STATUS {
        THREAD_SUCCESS = 0,
        NOT_STARTED = 1,
        SUSPENDED = 2,
        RESUMED = 3,
        RUNNING = 4,
        CANT_START = 5,
        ALREADY_SUSPENDED = 6,
        THREAD_TERMINATED = 7,
        NOT_SUPPORTED_ON_PLATFORM = 8,
        INVALID_REQUEST = 9, //! Trying to wait on a detached thread or detach a joined (WaitForThreadCompletion) thread
        ALREADY_RUNNING = 10
    };

    //! Start thread execution
    //! Passing the "this" pointer here is redundant:
    //! your WorkerFunction will be called in object context.
    virtual THREAD_STATUS StartThread(void *pParam = NULL, BOOL bStartSuspended = FALSE);
    //! Suspend thread
    virtual THREAD_STATUS SuspendThread();
    //! Resume thread
    virtual THREAD_STATUS ResumeThread();
    //! Return the thread handle
    virtual XPLAT_HANDLE GetThreadHandle();
    //! Check to see if thread is active
    virtual THREAD_STATUS GetThreadStatus();
    //! Wait for a thread to termintate
    virtual DWORD WaitForThreadCompletion(DWORD dwTimeout = INFINITE);
    //! Return a user defined thread type ID
    virtual int GetThreadType();
    //! Check to see if a thread is active
    static BOOL IsThreadActive(XPLAT_HANDLE hThread);
#if !defined(_WIN32)
    //! You must either detach the thread or join it for proper cleanup
    virtual int DetachThread();
#endif

protected:
    //! Called by CXPlatThread to start work in object context
    virtual unsigned WorkerFunction(void *pParam) = 0;
    //! Close handle
    void CloseThreadHandle();

protected:
    BOOL bSuspended;
    XPLAT_HANDLE hThreadHandle;
    void *pUserParm;
#if !defined(_WIN32)
    BOOL bDetached;
    BOOL bJoined;
#endif

private:
    //! Internal thread function
#if defined(_WIN32)
    static unsigned _stdcall ThreadFunc(void *pParam);
#else
    static unsigned ThreadFunc(void *pParam);
#endif
};

#endif
