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
//! CPServer.h: interface for the CPServer class.

#if !defined(AFX_CPSERVER_H__418CD0F7_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_)
#define AFX_CPSERVER_H__418CD0F7_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "stldef.h"
#include "CBaseSocket.h"
#include "CPConnection.h"
#include "CPConnectionHandler.h"
#include "CXPlatCriticalSection.h"
#include "CXPlatThread.h"

//! Pooled connection server class. Handles multiple TCP connections
//! with just a connection accept thread and an I/O thread, making it highly scalable
class CPServer : public CBaseSocket
{
//! constructor
public:
	CPServer(CP_CONNECTION_HANDLER_ARRAY &cPConnectionHandlers);
	virtual ~CPServer();  
public:
	    BOOL	    Listen(int nConnectionBacklog = SOMAXCONN);
        void        ShutdownAll();
        time_t      GetTimeStarted();

protected:
            //! Clean up array
virtual void			CleanupArray(BOOL bTerminateAll = FALSE);
            //! Accept connections
virtual BOOL	        Accept();
            //! Pure virtual functions allocate and dallocate a derivative of CConnectionSocket
            //! Redefine in derived class
virtual CPConnection	*AllocateSocketClass(struct sockaddr *pAddr) = 0;
            //! Listening thread
#if defined(_WIN32)
static  unsigned _stdcall AcceptConnection(void* pVoid);
#else
static  unsigned AcceptConnection(void* pVoid);
#endif

    //! data member
protected:
			//! Array of CPConnectionHandlers.  Use the one with the least connections.
	CP_CONNECTION_HANDLER_ARRAY		cHandlers;
        //! Array of CPConnection objects created
    vector<CPConnection *>			cSocketArray;
    XPLAT_HANDLE					hThread;
    time_t							tStarted;
	CXPlatCriticalSection			cCritSocketArray;
	int								iNumHandlers;
};

#endif //! !defined(AFX_CPSERVER_H__418CD0F7_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_)
