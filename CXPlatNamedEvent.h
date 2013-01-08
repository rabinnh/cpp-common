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
#if !defined(__CXPlatNamedEvent_H__)
#define __CXPlatNamedEvent_H__

#include "XPlat.h"

//! Cross playform synchronization object
class CXPlatNamedEvent
{
public:
    //! Constructor
#if defined(_WIN32)
    CXPlatNamedEvent(const char *szName, LPSECURITY_ATTRIBUTES lpSA = NULL);	//! Named Event (inter-process)
#else
    CXPlatNamedEvent(const char *szName);	//! Named Event (inter-process)
#endif
    //! Destructor
    ~CXPlatNamedEvent();
    //! Tries to obtain access
	void	Lock();
	//! Set the event
    void	SetEvent();

static	const char				*szEventSig;
#if defined(_WIN32)
    HANDLE						m_csHandle;
#else
	int							shared_mem_id;
	sem_t						*m_csHandle;
#endif
};

#endif
