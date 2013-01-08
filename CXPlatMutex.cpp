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
#else
#include "XPlat.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include "Crc32.h"
#endif

#include "CXPlatMutex.h"

const char	*CXPlatMutex::szMutexSig = "_XPLAT_MUTEX";

#define pthread_mutexattr_default NULL

// Constructor
CXPlatMutex::CXPlatMutex() 
{    
#if defined(_WIN32)
    m_csHandle = ::CreateMutex(NULL, FALSE, NULL);
#else
    m_csHandle = &mutex_data;
	::pthread_mutex_init(m_csHandle, pthread_mutexattr_default);
    lock_count = 0;
    p_lock_count = &lock_count;
    p_lock_id = &lock_id;
#endif
}

// Constructor
CXPlatMutex::CXPlatMutex(const char *szName) 
{    
#if defined(_WIN32)
    m_csHandle = ::CreateMutex(NULL, FALSE, szName);
#else
	CRC32				cCRC;
	unsigned long		lCRC = 0xFFFFFFFF;
	struct				shmid_ds shm_stat;

		// Create hashed key value from name by using CRC32
	lCRC = cCRC.CalcCRC((long unsigned int)lCRC, (unsigned char *)szName, (int)::strlen(szName));
	lCRC = cCRC.CalcCRC((long unsigned int)lCRC, (unsigned char *)szMutexSig, (int)::strlen(szMutexSig));

		// Obtain the shared memory
	shared_mem_id = ::shmget(lCRC, sizeof(pthread_mutex_t)+sizeof(pthread_t)+sizeof(int), 0666);
	if ( shared_mem_id == -1) {
      shared_mem_id = ::shmget(lCRC, sizeof(pthread_mutex_t), 0666 | IPC_CREAT);
	}
	m_csHandle = (pthread_mutex_t *) ::shmat(shared_mem_id, NULL, 0);
	p_lock_id = (pthread_t *)m_csHandle+sizeof(pthread_mutex_t);
	p_lock_count = (int *)p_lock_id+sizeof(pthread_t);

		// If first process to attach memory, init the mutex
	::shmctl(shared_mem_id, IPC_STAT, &shm_stat);
	if (shm_stat.shm_nattch == 1) {
    #ifndef __hpux1000
      // all but HP-UX 10.20
		::pthread_mutexattr_init(&mutex_shared_attr);
    #else
		::pthread_mutexattr_create(&mutex_shared_attr);
    #endif
#if !defined(__linux) && !defined(_AIX) && !defined(__hpux1000)
      ::pthread_mutexattr_setpshared(&mutex_shared_attr, PTHREAD_PROCESS_SHARED);
#endif
      ::pthread_mutex_init(m_csHandle, &mutex_shared_attr);
      ::pthread_mutexattr_destroy(&mutex_shared_attr);
      *p_lock_count = 0;
   }
#endif
}

// Destructor
CXPlatMutex::~CXPlatMutex()
{
#if defined(_WIN32)
    ::CloseHandle(m_csHandle);
#else
   if (m_csHandle == &mutex_data) {
      ::pthread_mutex_destroy( m_csHandle );
   }
#endif        
}

// Tries to obtain exclusive access
int CXPlatMutex::Lock(DWORD dwWait)
{
#if defined(_WIN32)
    ::WaitForSingleObject(m_csHandle, INFINITE);
#else
	if (*p_lock_count) {
        // note: do not code ::pthread_equal; its a macro on AIX
       if ( pthread_equal( *p_lock_id, ::pthread_self() )) {
           *p_lock_count = *p_lock_count + 1;
           return 0;
		}
	}

    // Timed wait?
    if (dwWait == INFINITE)
        ::pthread_mutex_lock(m_csHandle);
    else
        {   // How long should we wait?
        struct timespec sAbsWaitTime;
        ::GetAbstime(sAbsWaitTime, dwWait);
        return ::pthread_mutex_timedlock(m_csHandle, &sAbsWaitTime) == ETIMEDOUT ? XPLAT_TIMEOUT : 0;
    }
    
    
    *p_lock_count = *p_lock_count + 1;
    *p_lock_id = ::pthread_self();
    return 0;

#endif        
}

// Releases exclusive access
void CXPlatMutex::Unlock()
{
#if defined(_WIN32)
    ::ReleaseMutex(m_csHandle);
#else
    *p_lock_count = *p_lock_count - 1;
    if (!*p_lock_count) {
        ::pthread_mutex_unlock(m_csHandle);
    }
#endif        
};


#if defined(_WIN32)
HANDLE CXPlatMutex::GetMutexHandle()
#else
pthread_mutex_t	*CXPlatMutex::GetMutexHandle()
#endif
{
	return(m_csHandle);
};
