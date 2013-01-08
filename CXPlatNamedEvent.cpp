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
#include "XPlat.h"
#if !defined(_WIN32)
#include <semaphore.h>
#include "Crc32.h"
#include <sys/shm.h>
#endif

#include "CXPlatNamedEvent.h"

const char *CXPlatNamedEvent::szEventSig = "_XPLAT_EVENT";

// Constructor
#if defined(_WIN32)
CXPlatNamedEvent::CXPlatNamedEvent(const char *szName, LPSECURITY_ATTRIBUTES lpSA)
#else

CXPlatNamedEvent::CXPlatNamedEvent(const char *szName)
#endif
{
    string szEventName = szName;

    szEventName += szEventSig;
#if defined(_WIN32)
    m_csHandle = ::CreateEvent(lpSA, FALSE, FALSE, szEventName.c_str());
#else
    CRC32 cCRC;
    unsigned long lCRC = 0xFFFFFFFF;
    struct shmid_ds shm_stat;

    // Create hashed key value from name by using CRC32
    lCRC = cCRC.CalcCRC((long unsigned int) lCRC, (unsigned char *) szEventName.c_str(), (int) szEventName.length());

    // Obtain the shared memory
    int sh_created = 0;
    shared_mem_id = ::shmget(lCRC, sizeof(sem_t), 0666);
    if (shared_mem_id == -1)
    {
        shared_mem_id = ::shmget(lCRC, sizeof(sem_t), 0666 | IPC_CREAT);
        sh_created = 1;
    }
    m_csHandle = (sem_t *) ::shmat(shared_mem_id, NULL, 0);

    // If first process to attach memory, init the semaphore
    ::shmctl(shared_mem_id, IPC_STAT, &shm_stat);
    int tmp;
    if (shm_stat.shm_nattch == 1)
    { // this doesn't destory the semaphore on Linux, so we will loop to set the value to 1
        if (sh_created)
        {
            ::sem_destroy(m_csHandle);
        }
        ::sem_init(m_csHandle, 1, 1);
        ::sem_getvalue(m_csHandle, &tmp);
        for(; tmp > 1;)
        {
            ::sem_wait(m_csHandle);
            ::sem_getvalue(m_csHandle, &tmp);
        }
    }
#endif
}

// Destructor

CXPlatNamedEvent::~CXPlatNamedEvent()
{
#if defined(_WIN32)
    if (m_csHandle)
        ::CloseHandle(m_csHandle);
#else
    struct shmid_ds shm_stat;
    ::shmctl(shared_mem_id, IPC_STAT, &shm_stat);
    if (shm_stat.shm_nattch == 1)
    {
        ::sem_destroy(m_csHandle);
    }
#endif        
}

// Tries to obtain exclusive access

void CXPlatNamedEvent::Lock()
{
#if defined(_WIN32)
    ::WaitForSingleObject(m_csHandle, INFINITE);
#else
    int tmp;
    ::sem_getvalue(m_csHandle, &tmp);
    ::sem_wait(m_csHandle);
    ::sem_getvalue(m_csHandle, &tmp);
#endif        
}

// Releases exclusive access

void CXPlatNamedEvent::SetEvent()
{
#if defined(_WIN32)
    ::SetEvent(m_csHandle);
#else
    int tmp;
    ::sem_getvalue(m_csHandle, &tmp);
    ::sem_post(m_csHandle);
    ::sem_getvalue(m_csHandle, &tmp);
#endif        
};
