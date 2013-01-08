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
#if !defined(__CXPlatMutex_H__)
#define __CXPlatMutex_H__

#include "XPlat.h"

//! Cross platform mutex object
class CXPlatMutex
{
public:
    //! Constructor
    CXPlatMutex();	//! intra-process mutex
    //! Constructor
    CXPlatMutex(const char *szName);	//! inter-process named mutex
    //! Destructor
    ~CXPlatMutex();
    //! Tries to obtain exclusive access
    int 	Lock(DWORD dwWait = INFINITE);
    //! Releases exclusive access
    void	Unlock();

#if defined(_WIN32)
	HANDLE			GetMutexHandle();
#else
	pthread_mutex_t	*GetMutexHandle();
#endif

protected:

static	const char			*szMutexSig;
#if defined(_WIN32)
    HANDLE					m_csHandle;
#else
    pthread_mutex_t			mutex_data;
    pthread_mutex_t			*m_csHandle;
	int						shared_mem_id;
	pthread_mutexattr_t	    mutex_shared_attr;
    pthread_t               lock_id;
    pthread_t *             p_lock_id;
    int                     lock_count;
    int *                   p_lock_count;
#endif
};

#endif
