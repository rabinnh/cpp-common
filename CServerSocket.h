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
//! Server socket listens to establish a connection


#ifndef _CSERVERSOCKET_H
#define _CSERVERSOCKET_H

#include "CXPlatCriticalSection.h"
#include "CXPlatThread.h"
#include "CConnectionSocket.h"

//! TCP socket server class
class CServerSocket : public CBaseSocket
{
    //! constructor
public:
    CServerSocket();
    virtual ~CServerSocket();
    //! methods

    BOOL    Listen(int nConnectionBacklog = 5);
    void    ShutdownAll();
    time_t  GetTimeStarted();
    //! Wait for listening thread to exit
    void    WaitForExit();

protected:
    //! Clean up array
    virtual void CleanupArray(BOOL bTerminateAll = FALSE);
    //! Accept connections
    virtual BOOL Accept();
    //! Pure virtual functions allocate and dallocate a derivative of CConnectionSocket
    //! Redefine in derived class
    virtual CConnectionSocket *AllocateSocketClass(struct sockaddr *pAddr) = 0;
    //! Listening thread
#if defined(_WIN32)
    static unsigned _stdcall AcceptConnection(void* pVoid);
#else
    static unsigned AcceptConnection(void* pVoid);
#endif

    //! data member
protected:
    //! Array of CConnectionSocket objects created
    vector<void *> cSocketArray;
    XPLAT_HANDLE hThread;
    time_t tStarted;
    CXPlatCriticalSection cCritSocketArray;
};

#endif	//!!_CSERVERSOCKET_H
