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
#include <stdafx.h>
#endif

//#include "CXPlatEvent.h"
#include "CXPlatCriticalSection.h"
#include <iostream>

// Constructor

CXPlatCriticalSection::CXPlatCriticalSection()
{
#if defined(_WIN32)
    InitializeCriticalSection(&m_csHandle);
#else
    pthread_mutex_init((pthread_mutex_t *) &m_csHandle, pthread_mutexattr_default);
    lock_count = 0;
#endif
}

// Destructor

CXPlatCriticalSection::~CXPlatCriticalSection()
{
#if defined(_WIN32)
    DeleteCriticalSection(&m_csHandle);
#else
    pthread_mutex_destroy(&m_csHandle);
#endif        
}

// Tries to obtain exclusive access

int CXPlatCriticalSection::Lock(DWORD dwWait)
{
#if defined(_WIN32)
    EnterCriticalSection(&m_csHandle);
    return( 1);
#else
    // This lock_count stuff is attempting to replicate Windows, which allows multiple
    // critical section locks by the same thread (which is bad programming).  
    // The problem is that this only handles that occurrence by one thread of the many in the program.
    // Don't complicate stuff.  Lock it and unlock it.
    /*    
        if (lock_count)
        {
            if (pthread_equal(lock_id, pthread_self()))
            {
                lock_count++;
                return (1);
            }
        }

        ::pthread_mutex_lock(&m_csHandle);
        lock_count++;
        lock_id = pthread_self();
     */
    if (dwWait == INFINITE)
        ::pthread_mutex_lock(&m_csHandle);
    else
    {   // How long should we wait?
        struct timespec sAbsWaitTime;
        ::GetAbstime(sAbsWaitTime, dwWait);
        return ::pthread_mutex_timedlock(&m_csHandle, &sAbsWaitTime) == ETIMEDOUT ? XPLAT_TIMEOUT : 0;
    }

    return(1);
#endif        
}

// Releases exclusive access

int CXPlatCriticalSection::Unlock()
{
#if defined(_WIN32)
    LeaveCriticalSection(&m_csHandle);
    return( 1);
#else
    /*    
        if (pthread_equal(lock_id, pthread_self()))
        {
            if (--lock_count < 0)
                lock_count = 0;
            else if (!lock_count)
            {
                return ( 0 == ::pthread_mutex_unlock(&m_csHandle));
            }
            return (1);
        }
        return (0);
     */
    return( 0 == ::pthread_mutex_unlock(&m_csHandle));

#endif        
}
