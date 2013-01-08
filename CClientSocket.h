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
//! Client socket 
//
//	Created: Richard A. Bross
//
//! Implements a client connection.  You should derive a class from this class
//! which implements a ProcessData method for incoming data and also some
//! external method for sending data (internally, you can use the SendDataOut
//! method).
//

#ifndef _CCLIENTSOCKET_H
#define _CCLIENTSOCKET_H

#include "CXPlatCriticalSection.h"
#include "CXPlatThread.h"
#include "CXPlatEvent.h"
#include "CConnectionSocket.h"

class CClientSocket : public CConnectionSocket
{
    //! constructor
public:
    CClientSocket();
    virtual ~CClientSocket();
    //! Connect to a server
    //! WARNING - if you are going to reuse this object for multiple connections, then you MUST call 
    //!           TerminateConnection after each connection transaction is complete to clean up resources.
    virtual BOOL ConnectToServer(LPCSTR lpszHostAddress, UINT nHostPort, DWORD dwTimeout = INFINITE);
    virtual BOOL ConnectToUnixServer(const char *szUnixSocket, DWORD dwTimeout = INFINITE);
    time_t GetTimeStarted();
    //! Last error reported
    virtual int GetConnectionError();
    //! Set socket options - must be called BEFORE ConnectToServer
    //! Normally, the defaults are fine unless you are going to create and tear down lots of connections
    void SetOptions(bool bReuse, int iLinger, bool bTcpNoDelay = false);

protected:
    //! Process connections
    virtual BOOL AttemptConnection();
    //! Spawn the connection thread
    virtual BOOL InitiateConnection();
    //! Connect attempt successful?
    virtual BOOL LastConnectAttemptSuccessful();
    //! Distribute to either the TCP or UNIX connection code
    virtual BOOL ConnectDispatcher(DWORD dwTimeout);
    //! Client connect thread
#if defined(_WIN32)
    static unsigned _stdcall ConnectThread(void* pVoid); //! Actually does the work
#else
    static unsigned ConnectThread(void* pVoid); //! Actually does the work
#endif
    //! data member
protected:
    BOOL bDestroying;
    time_t tStarted;
    XPLAT_HANDLE hClientThread;
    BOOL bConnectAttempt;
    int iConnectError;
    bool bReuse;
    int iLinger;
    bool bTcpNoDelay;
    CXPlatCriticalSection cCritSection;
#if !defined(_WIN32)
    CXPlatEvent cConnectThreadDoneEvent;
#endif
};

#endif	//! !_CCLIENTSOCKET_H
