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
#if !defined(__CXPlatEvent_H__)
#define __CXPlatEvent_H__

#include "XPlat.h"

#if !defined(_WIN32)
#include "CXPlatCriticalSection.h"
#endif

#define USE_SEMAPHORE
//! Cross platform event synchronization class

class CXPlatEvent {
public:

    //! Constructor
    CXPlatEvent(BOOL bManualReset = FALSE); //! Cross platform mutex
    //! Destructor
    ~CXPlatEvent();
    //! Tries to obtain access
    int Lock(DWORD dwWait = INFINITE);
    //! Set the event
    void SetEvent();
    //! Reset event to non-signalled
    void ResetEvent();
    //! Was the event created successfully
    BOOL WasCreatedSuccessfully();

protected:
    BOOL bEventCreated;
    BOOL bManualReset;

#if defined(_WIN32)
    HANDLE m_csHandle;
#else
    pthread_cond_t m_csHandle;
#ifndef USE_SEMAPHORE    
    CXPlatCriticalSection cCondMutex;
    BOOL bEventPosted;
#else    
    sem_t   ptSem;
#endif    
    int iSetCount;
#endif
};

#endif
