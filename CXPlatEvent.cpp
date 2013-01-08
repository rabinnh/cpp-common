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
#include "CXPlatEvent.h"
#include <semaphore.h>


// This should ALWAYS be defined for Linux!
// This does a "double check" because pthreads do not allow a mutex to be signalled before it is waited on.
// Our solution of an iSetEvent counter works 99.99% of the time.  However, in a test with 50,000 plus loops,
// we did see one case where a deadlock snuck in because the wait was before we incremented the counter.
// That's actually 99.998% of the time
#define SUPER_SAFE  

// Constructor

CXPlatEvent::CXPlatEvent(BOOL bManualReset)
{
    bEventCreated = FALSE;
    this->bManualReset = bManualReset;
#if defined(_WIN32)
    m_csHandle = ::CreateEvent(NULL, bManualReset, FALSE, NULL);
    if (m_csHandle)
        bEventCreated = TRUE;
#else
    iSetCount = 0;
#ifndef USE_SEMAPHORE    
    bEventPosted = FALSE;
    if (::pthread_cond_init(&m_csHandle, pthread_condattr_default) == 0)
#else        
    if (::sem_init(&ptSem, iSetCount, 0));
#endif    
        bEventCreated = TRUE;
#endif
}

// Destructor

CXPlatEvent::~CXPlatEvent()
{
#if defined(_WIN32)
    ::CloseHandle(m_csHandle);
#else
#ifndef USE_SEMAPHORE    
    ::pthread_cond_destroy(&m_csHandle);
#else    
    ::sem_destroy(&ptSem);
#endif        
#endif        
}


// Reset event to non-signalled

void CXPlatEvent::ResetEvent()
{
#if defined(_WIN32)
    ::ResetEvent(m_csHandle);
#else
#ifndef USE_SEMAPHORE
    bEventPosted = FALSE;
    iSetCount = 0;

#else    
    // This will decrement without blocking and return an error
    while(::sem_trywait(&ptSem) != -1);
#endif    
#endif
};

#ifndef _DEBUG
//#define _DEBUG
#endif

// Tries to obtain exclusive access

int CXPlatEvent::Lock(DWORD dwWait)
{
#if defined(_WIN32)
    return(::WaitForSingleObject(m_csHandle, dwWait));
#else
    int iReturn;
    if (::sem_trywait(&ptSem) == -1)    // Not signalled
    {
        if (dwWait == INFINITE)
            ::sem_wait(&ptSem);
        else
        {
            struct timespec sAbsWaitTime;
            ::GetAbstime(sAbsWaitTime, dwWait);
            int iRet = ::sem_timedwait(&ptSem, &sAbsWaitTime);
            if (iRet == -1)
                iRet = errno;
            return iRet == ETIMEDOUT ? XPLAT_TIMEOUT : iRet;
        }
    }
#endif      
}

    



// Releases exclusive access

void CXPlatEvent::SetEvent()
{
#if defined(_WIN32)
    ::SetEvent(m_csHandle);
#else
    iSetCount++;
#ifndef USE_SEMAPHORE
    // Don't allow then to take us to less than 0
    ::pthread_cond_broadcast(&m_csHandle);
#else
    ::sem_post(&ptSem);
#endif    
#endif   
};


// Was the event created successfully

BOOL CXPlatEvent::WasCreatedSuccessfully()
{
    return(bEventCreated);
};



