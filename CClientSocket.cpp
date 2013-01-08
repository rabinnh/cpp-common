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
// Client socket
//
//	Created: Richard A. Bross
//
// Implements a client connection.  You should derive a class from this class
// which implements a ProcessData method for incoming data and also some
// external method for sending data (internally, you can use the SendDataOut
// method).
//
#include "XPlat.h"
#include "CXPlatThread.h"
#include "CBaseSocket.h"
#include "CConnectionSocket.h"
#include "CClientSocket.h"

// constructor

CClientSocket::CClientSocket() : CConnectionSocket(NULL)
{
    tStarted = time(NULL);
    memset(&hClientThread, 0, sizeof(hClientThread));
    bDestroying = bReuse = FALSE;
    iLinger = -1;
    iConnectError = 0;
    bTcpNoDelay = false;
}

CClientSocket::~CClientSocket()
{
    bDestroying = TRUE;

    // Release connect thread handle
    if (sSocket && sSocket != SOCKET_ERROR)
        CloseSocket();
    if (hClientThread)
    {
        cCritSection.Lock();
#if defined(_WIN32)
        ::WaitForSingleObject(hClientThread, INFINITE);
        ::CloseHandle(hClientThread); // _beginthreadex doesn't auto close the handle
#else
        ::pthread_join(hClientThread, NULL);
#endif
        memset(&hClientThread, 0, sizeof(hClientThread));
        cCritSection.Unlock();
    };
}

// Set socket options - must be called BEFORE ConnectToServer

void CClientSocket::SetOptions(bool bReuse, int iLinger, bool bTcpNoDelay)
{
    this->bReuse = bReuse;
    this->iLinger = iLinger;
    this->bTcpNoDelay = bTcpNoDelay;
}




// Distribute to either the TCP or UNIX connection code
BOOL CClientSocket::ConnectDispatcher(DWORD dwTimeout)
{
    // Use a thread to attempt connection
    if (!InitiateConnection())
    {
        cCritSection.Unlock();
        // Close the unused socket
        UnprotectedCloseSocket();
        return(FALSE);
    };

    // Wait for the thread to complete
#if defined(_WIN32)
    if (::WaitForSingleObject(hClientThread, dwTimeout) == XPLAT_TIMEOUT)
#else
    // An event is used instead of pthread_join on Linux so we can timeout
    if (cConnectThreadDoneEvent.Lock(dwTimeout) == XPLAT_TIMEOUT)
#endif
    { // Timed out
        CloseSocket();
#if defined(_WIN32)
        ::WaitForSingleObject(hClientThread, INFINITE);
        ::CloseHandle(hClientThread);
#else   // *nix
        if (hClientThread)
        {
#if defined(__linux)    // Linux
        // linux sometimes hangs on the __connect() when the socket is closed from another thread!
            ::pthread_cancel(hClientThread);
#endif
            ::pthread_join(hClientThread, NULL);
#endif
        }
        memset(&hClientThread, 0, sizeof(hClientThread));
        cCritSection.Unlock();
        // Close the unused socket
        UnprotectedCloseSocket();
        return(FALSE);
    };

#if !defined(_WIN32)

    if (hClientThread)
        ::pthread_join(hClientThread, NULL); // Must do this to clean up the thread
#endif

    // Release connect thread handle
    if (hClientThread)
    {
#if defined(_WIN32)
        ::CloseHandle(hClientThread);
#endif
        memset(&hClientThread, 0, sizeof(hClientThread));
    };
    
    // Successful?
    if (!LastConnectAttemptSuccessful())
    {
        cCritSection.Unlock();
        // Close the unused socket
        UnprotectedCloseSocket();
        return(FALSE);
    };

    if (IsConnected())
    {
        cCritSection.Unlock();
        // Close the unused socket
        UnprotectedCloseSocket();
        return(FALSE);
    };

    cCritSection.Unlock();

    // Start the socket servicing the client
    ServiceConnection(sSocket);

    return(TRUE);
}


// Connect to a Unix domain socket server
BOOL CClientSocket::ConnectToUnixServer(const char *szUnixPath, DWORD dwTimeout)
{
    bConnectAttempt = FALSE;
    
    cCritSection.Lock();

    if (bDestroying)
    {
        cCritSection.Unlock();
        return(FALSE);
    };

    // Already connected?
    if (IsConnected())
    {
        cCritSection.Unlock();
        TerminateConnection();
    };

    // Create the socket
    if (!Create(AF_UNIX, SOCK_STREAM, 0, 0, szUnixPath, FALSE, FALSE))
    {
        ::printf("Unable to create the Unix socket: %s\n", szUnixPath);
        cCritSection.Unlock();
        return(FALSE);
    }

    // Set up Unix socket address
    ::memset(&sAddr, 0, sizeof(sAddr));
    ::memset(&sUAddr, 0, sizeof(sUAddr));
     sUAddr.sun_family = AF_UNIX;
    ::strcpy(sUAddr.sun_path, szUnixPath);

     return(ConnectDispatcher(dwTimeout));
}


// Connect to a TCP server
BOOL CClientSocket::ConnectToServer(LPCSTR lpszHostAddress, UINT nHostPort, DWORD dwTimeout)
{
    struct hostent *lphost;

    bConnectAttempt = FALSE;

    cCritSection.Lock();

    if (bDestroying)
    {
        cCritSection.Unlock();
        return(FALSE);
    };

    // Already connected?
    if (IsConnected())
    {
        cCritSection.Unlock();
        TerminateConnection();
    };

    // Make sure we have a host address
    if (!lpszHostAddress)
    {
        cCritSection.Unlock();
        return(FALSE);
    };

    // Create our socket without a "bind".  ::connect will establish the port.
    Create(AF_INET, SOCK_STREAM, 0, 0, NULL, bReuse, FALSE, iLinger, bTcpNoDelay);

    // Set up socket structure
    ::memset(&sAddr, 0, sizeof(sAddr));
    ::memset(&sUAddr, 0, sizeof(sUAddr));

    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = inet_addr(lpszHostAddress);

    // Name instead of IP address?
    if (sAddr.sin_addr.s_addr == INADDR_NONE)
    {
        PROTECT_CALL
        lphost = ::gethostbyname(lpszHostAddress);

        if (lphost != NULL)
        {
            sAddr.sin_addr.s_addr = ((struct in_addr *) lphost->h_addr)->s_addr;
            UNPROTECT_CALL
        }
        else
        {
            UNPROTECT_CALL
            cCritSection.Unlock();
            // Close the unused socket
            UnprotectedCloseSocket();
            return(FALSE);
        };
    }

    // Load port
    sAddr.sin_port = htons((u_short) nHostPort);
    
    return(ConnectDispatcher(dwTimeout));

};


// Return the time the server was started

time_t CClientSocket::GetTimeStarted()
{
    return(tStarted);
};


// Listen for someone trying to connect

BOOL CClientSocket::InitiateConnection()
{
    // Already connected?
    if (hClientThread)
    {
        if (CXPlatThread::IsThreadActive(hClientThread))
            return(TRUE);
        else
        {
#if defined(_WIN32)
            ::CloseHandle(hClientThread);
#endif
            memset(&hClientThread, 0, sizeof(hClientThread));
        };
    };


#if defined(_WIN32)
    unsigned uThread;

    hClientThread = (HANDLE) ::_beginthreadex(NULL,
        0,
        ConnectThread,
        (void *) this,
        0, // initflag = 0, start suspended
        &uThread);
    if (hClientThread)
    {
        ::ResumeThread(hClientThread);
        return(TRUE);
    };
#else
    int iReturn;
    iReturn = ::pthread_create(&hClientThread, pthread_attr_default, (START_ROUTINE) ConnectThread, (void *) this);
#endif

    return(TRUE);
}


// Thread to accept connections

unsigned CClientSocket::ConnectThread(void* pVoid)
{
    unsigned uReturn;
    CClientSocket *pCSocket = (CClientSocket *) pVoid;

    pCSocket->bConnectAttempt = pCSocket->AttemptConnection();
    uReturn = pCSocket->bConnectAttempt;
#if !defined(_WIN32)
    pCSocket->cConnectThreadDoneEvent.SetEvent();
#endif

    return(uReturn);
};


// Thread to accept connections

BOOL CClientSocket::AttemptConnection()
{
    int iReturn;

    // Thread to connect
    try
    {   // TCP or Unix
        if (sAddr.sin_port != 0)
            iReturn = ::connect(sSocket, (struct sockaddr *) &sAddr, sizeof(sAddr));
        else
            iReturn = ::connect(sSocket, (struct sockaddr *) &sUAddr, strlen(sUAddr.sun_path) + sizeof(sUAddr.sun_family));
    }
    catch(...)
    {
        iReturn = SOCKET_ERROR;
    }
    if (iReturn == SOCKET_ERROR)
        iConnectError = XPLAT_WSAGETLASTERR;
    else
        iConnectError = 0;
    return(iReturn != SOCKET_ERROR);
};



// Connect attempt successful?

BOOL CClientSocket::LastConnectAttemptSuccessful()
{
    return(bConnectAttempt);
};


// Return the last connection error

int CClientSocket::GetConnectionError()
{
    return(iConnectError);
};
