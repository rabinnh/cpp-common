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
#if !defined(__CXPlatCriticalSection_H__)
#define __CXPlatCriticalSection_H__

#include "XPlat.h"

class CXPlatEvent;

//! Cross plaform synchronization class
//!
//! On Windows, critical sections are "cheaper" than interprocess mutexes (mutii?)
class CXPlatCriticalSection
{
friend class CXPlatEvent;
public:
    //! Constructor
    CXPlatCriticalSection();
    //! Destructor
    ~CXPlatCriticalSection();
    //! Tries to obtain exclusive access
    int		Lock(DWORD dwWait = INFINITE);
    //! Releases exclusive access
    int		Unlock();

protected:

#if defined(_WIN32)
    CRITICAL_SECTION	m_csHandle;
#else
    pthread_mutex_t		m_csHandle;
    pthread_t           lock_id;
    int                 lock_count;
#endif
};

#endif
