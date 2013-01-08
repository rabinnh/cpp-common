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
// CConnectionSocket.cpp file
//
//	Created: Richard A. Bross
//
#if defined(_WIN32)
#include "stdafx.h"
#endif
#include "XPlat.h"
#include "CBaseSocket.h"
#include "CConnectionSocket.h"
#include <syslog.h>
#include "CProfileTimer.h"
#include <poll.h>

// Constructor

CConnectionSocket::CConnectionSocket(CBaseSocket *pOwner, struct sockaddr *pAddr) : CBaseSocket()
{
    memset(&hThread, 0, sizeof(hThread));
    cOwner = pOwner;
    memset(&sAddr, 0, sizeof(sAddr));
    memset(&sUAddr, 0, sizeof(sUAddr));
    if (pAddr)
        memcpy(&sAddr, pAddr, sizeof(sAddr));
    eRequestClose = CLOSE_ACTIVE;
    if (cOwner)
        iAF = pOwner->GetSocketType();
}

CConnectionSocket::~CConnectionSocket()
{
    TerminateConnection();
}


// Requests that the handleconenction thread close
// NOTES:
//  We used to just close the socket.  However, data waiting in the socket buffer was lost.
//  We still want the thread to exit on an invalid socket, since the other side may close the connection.
//  Now we set a flag and let the HandleConnection thread get remaining data.
//  If a user is calling this and they are stupid enough to call SendDataOut at the same time,
//  tough luck, the connection will be closed after 50 ms max.

void CConnectionSocket::TerminateConnection(ECLOSE_TYPE eRequest)
{
    // Ask the connection thread to close
    CProfileTimer::SetTimestamp("CCS-CLOSE");
    eRequestClose = eRequest;
    if (hThread)
    {
#if defined(_WIN32)
        ::WaitForSingleObject(hThread, INFINITE);
        ::CloseHandle(hThread); // _beginthreadex doesn't auto close the handle
#else
        // Make sure thread finishes
        int iRet;
        if ((iRet = ::pthread_join(hThread, NULL)) != 0)
        { // EDEADLK (35) will occur if some bozo calls this routine from the thread itself
            ::openlog("connectionsocket", LOG_PID, LOG_LOCAL0);
            char strLogEntry[1024];
            ::sprintf(strLogEntry, "pthread_join error could cause memory leak.  Error number: %d", iRet);
            ::syslog(LOG_CRIT, "%s", strLogEntry);
            ::closelog();
        }
        else
        { // Only on successs
            ::memset(&hThread, 0, sizeof(hThread));
        }

#endif
    };
    // Close the socket
    CloseSocket();
};


// Starts thread to service connection.  Returns thread handle.

XPLAT_HANDLE CConnectionSocket::ServiceConnection(SOCKET sNewSocket)
{
    // Timestamp
    tConnected = time(NULL);

    // Already servicing a connection?
    if (hThread)
    {
        if (CXPlatThread::IsThreadActive(hThread))
            return(hThread);
        else
        {
#if defined(_WIN32)
            ::CloseHandle(hThread);
#endif
            ::memset(&hThread, 0, sizeof(hThread));
        };
    };

    // Assign socket
    sSocket = sNewSocket;


#if defined(_WIN32)
    // Begin the servicing thread
    unsigned pThread;

    hThread = (HANDLE) ::_beginthreadex(NULL,
        0,
        ServiceConnectionThread,
        (void *) this,
        1,
        &pThread);
#else
    int iReturn;
    iReturn = ::pthread_create(&hThread, pthread_attr_default, (START_ROUTINE) ServiceConnectionThread, (void *) this);
#endif

    return(hThread);
};




// Send data
BOOL CConnectionSocket::SendDataOut(LPSTR lpData, int nPacketSize, int nTotalLength)
{
    int nDataSent = 0;
    int nReturn = 0;
#if defined(__linux)
    int nFlags = 0x4000; // MSG_NOSIGNAL
#else
    int nFlags = 0;
#endif
//    fd_set writesets;
    struct pollfd fds[1];
//    int nReturnSet = SOCKET_ERROR;
#if defined(__linux)	
    //	In Linux, doing a close	on a socket in a thread doesn't 
    //  seem to always unblock select or listen in another
    //  thread.  So for Linux, we'll timeout after .5 seconds and loop
//    struct timeval sTv;
    // 3 ms.  This is the time that we wait for the output buffer to have some space.  Probably almost never have to wait.
//    long lUSec = 3000;
    long lUSec = 3;
#endif

    // Make sure pointer isn't NULL
    if (!lpData)
        return FALSE;

    // Send data
//    FD_ZERO(&writesets);
    fds[0].fd = sSocket;
    fds[0].events = POLLOUT | POLLWRBAND; // POLLOUT == POLLWRNORM 

    while(TRUE)
    {
        cCriticalSocket.Lock();
        if (sSocket == SOCKET_ERROR)
        {
            cCriticalSocket.Unlock();
            return(FALSE);
        };
//        FD_SET(sSocket, &writesets);
        cCriticalSocket.Unlock();

        // Anything waiting to be sent?
#if defined(__linux)
//        sTv.tv_sec = 0;
//        sTv.tv_usec = lUSec;
        try
        {
//            nReturnSet = ::select(sSocket + 1, NULL, &writesets, NULL, &sTv);
            nReturn = ::poll(fds, 1, lUSec);
        }
        catch(...)
        {
            nReturn = SOCKET_ERROR;
        }
#else
        nReturnSet = ::select((int) sSocket + 1, NULL, (XPLAT_FD_SET_PTR) & writesets, (XPLAT_FD_SET_PTR) NULL, NULL);
#endif
        if (nReturn == SOCKET_ERROR)
        {
            CloseSocket();
            return(FALSE);
        };

        cCriticalSocket.Lock();
        if (sSocket == SOCKET_ERROR)
        {
            cCriticalSocket.Unlock();
            return(FALSE);
        };
//        nReturn = FD_ISSET(sSocket, &writesets);
        cCriticalSocket.Unlock();
        if (!nReturn && (fds[0].revents & POLLHUP || fds[0].revents & POLLERR)) 
              return(FALSE);
        if (nReturn)
        {   // Other end closed
            // If not one of the "ok to write" events, loop.  Is this possible at this point
            if (!fds[0].revents & POLLWRBAND && !fds[0].revents & POLLOUT) 
                continue;
            // Send the whole packet to the client if the packet is big enough
            cCriticalSocket.Lock();
            if (sSocket == SOCKET_ERROR)
            {
                cCriticalSocket.Unlock();
                return(FALSE);
            };
            if (nDataSent + nPacketSize >= nTotalLength)
            {
                try
                {
                    nReturn = ::send(sSocket, (LPSTR) lpData + nDataSent, nTotalLength - nDataSent, nFlags);
                }
                catch(...)
                {
                    nReturn = SOCKET_ERROR;
                }
                cCriticalSocket.Unlock();
                if (nReturn == SOCKET_ERROR)
                {
                    nReturn = XPLAT_WSAGETLASTERR;
                    if (nReturn != XEWOULDBLOCK && // would block
                        nReturn != XEINPROGRESS) // in progress
                    { // error return
                        CloseSocket();
                        return(FALSE);
                    }
                };
                nDataSent += (nTotalLength - nDataSent);
                break;
            }
            else
            {
                try
                {
                    nReturn = ::send(sSocket, lpData + nDataSent, nPacketSize, nFlags);
                }
                catch(...)
                {
                    nReturn = SOCKET_ERROR;
                }
                cCriticalSocket.Unlock();
                if (nReturn == SOCKET_ERROR)
                {
                    nReturn = XPLAT_WSAGETLASTERR;
                    if (nReturn != XEWOULDBLOCK && // would block
                        nReturn != XEINPROGRESS) // in progress
                    { // error return
                        CloseSocket();
                        return(FALSE);
                    }
                };
                nDataSent += nPacketSize;
            }
            // It was all sent
            if (nDataSent >= nTotalLength)
                break;
            // We only have to worry about this for partial data
            CProfileTimer::SetTimestamp("CCS-SEND");
        }
        else
        {   // If nothing to write, it's been 6ms since the last write or 50 ms since we were asked to close
            if (eRequestClose == CLOSE_IMMEDIATE || // Close right now
                (eRequestClose == CLOSE_WAIT && // Wait for data.  1ms (6 with the select call) since last send or 50ms for any send
                (CProfileTimer::GetElapsedMS("CCS-SEND", "NowSend", true) > 1 || CProfileTimer::GetElapsedMS("CCS-CLOSE", "NowSend", false) > 50)))
            {
                break;
            }
        }
        // Clean up
        cCriticalSocket.Lock();
        if (sSocket == SOCKET_ERROR)
        {
            cCriticalSocket.Unlock();
            return(FALSE);
        };
//        FD_ZERO(&writesets);
        cCriticalSocket.Unlock();
    }

    return(nDataSent == nTotalLength);
}



// Receive the data from socket, external buffer

int CConnectionSocket::ReceiveData(void *lpBuf, int nBufLen)
{
    int iReturn;
#if defined(__linux)
    int nFlags = 0x4000; // MSG_NOSIGNAL
#else
    int nFlags = 0;
#endif

    // Get any data sent over the connection
    cCriticalSocket.Lock();
    try
    {
        iReturn = ::recv(sSocket, (LPSTR) lpBuf, nBufLen, nFlags);
    }
    catch(...)
    {
        iReturn = SOCKET_ERROR;
    }
    cCriticalSocket.Unlock();

    return(iReturn);
}

// Receive the data from socket, internal buffer

int CConnectionSocket::ReceiveData()
{
    return(ReceiveData(ucBuffer, CS_BUFFER_SIZE));
}


// Called by ServiceConnectionThread to actually do the work

int CConnectionSocket::HandleConnection()
{
//    fd_set readsets;
    struct pollfd fds[1];
    int nReturn;
    int nRevLength;
    int nPacket = 2048;
    unsigned char *ucHS;
#if defined(__linux)
    // Linux "select()" does not unblock when the socket is closed.  So we set a timeout.
//    struct timeval sTv;
//    long lUSec = 3000; // If the socket gets closed, this will shut it down within 3ms
    long lUSec = 3; // If the socket gets closed, this will shut it down within 3ms
#endif
    try
    {
        // Send handshake, if derived class wants us to
        if (GetHandshake(&ucHS, nReturn))
            SendDataOut((char *) ucHS, nPacket, nReturn);

        // Clear sets
//        FD_ZERO(&readsets);
        fds[0].fd = sSocket;
        fds[0].events = POLLIN | POLLRDBAND;    // POLLRDNORM == POLLIN.  POLLRDHUP is only available if _GNU_SOURCE is defined before headers

        // Read the data
        while(TRUE)
        {
            cCriticalSocket.Lock(); // Lock
            // Someone close it?
            if (sSocket == SOCKET_ERROR)
            {
                cCriticalSocket.Unlock(); // if Unlock
                Disconnected();
                return(0);
            };

            // Init sets
//            FD_SET(sSocket, &readsets);
//            cCriticalSocket.Unlock(); // Unlock

            // Blocking call
#if defined(__linux)
            // Reset every time because the Linux "select()" modifies the value
//            sTv.tv_sec = 0;
//            sTv.tv_usec = lUSec;
            try
            {
//                nReturn = ::select(sSocket + 1, &readsets, NULL, NULL, &sTv);
                nReturn = ::poll(fds, 1, lUSec);
            }
            catch(...)
            {
                nReturn = SOCKET_ERROR;
            }
#else
            nReturn = ::select((int) sSocket + 1, (XPLAT_FD_SET_PTR) & readsets, NULL, (XPLAT_FD_SET_PTR) NULL, NULL);
#endif
            // Someone close it or an error
//            cCriticalSocket.Lock();
            cCriticalSocket.Unlock();
            if (nReturn == SOCKET_ERROR)
            {
                if (sSocket != SOCKET_ERROR)
                    CloseSocket();
                Disconnected();
                return(0);
            };

//            nReturn = FD_ISSET(sSocket, &readsets);
//            cCriticalSocket.Unlock();
            if (!nReturn && (fds[0].revents & POLLHUP || fds[0].revents & POLLERR)) 
            {
                Disconnected();
                return(0);
            }
            // We have data
            if (nReturn)
            {   // If not one of the "ok to read" events, loop.  Is this possible at this point
                if (!fds[0].revents & POLLRDBAND && !fds[0].revents & POLLIN) 
                    continue;
                nRevLength = ReceiveData();
                if (nRevLength == SOCKET_ERROR || nRevLength == 0)
                {
                    nReturn = XPLAT_WSAGETLASTERR;
                    if (nReturn != XEWOULDBLOCK && // would block
                        nReturn != XEINPROGRESS) // in progress
                    { // error return, probably closed and ended nornally
                        if (sSocket != SOCKET_ERROR)
                            CloseSocket();
                        Disconnected();
                        return(0);
                    }
                }
                // This should be impossible
//                if (nRevLength > CConnectionSocket::CS_BUFFER_SIZE)
//                {
//                    Disconnected();
//                    return(0);
//                };
                // Echo data
                if (nRevLength > 0)
                {   // Make sure our socket isn't closed. If we're in our destructor, the derived class
                    // is already destroyed and ProcessData will be calling a pure virtual function.
//                    if (sSocket == SOCKET_ERROR)
//                    {
//                        Disconnected();
//                        return(0);
//                    };
                    // Process the data
                    ProcessData(ucBuffer, nRevLength);
                    CProfileTimer::SetTimestamp("CCS-RECV");
                }
            }
            else
            { // If nothing to read, it's been 6ms since the last read or 50 ms since we were asked to close
                if (eRequestClose == CLOSE_IMMEDIATE || // Close right now
                    (eRequestClose == CLOSE_WAIT && // Wait for data.  1ms (6 with the select call) since last receive or 50ms for any receive
                    (CProfileTimer::GetElapsedMS("CCS-RECV", "NowRecv", true) > 1 || CProfileTimer::GetElapsedMS("CCS-CLOSE", "NowRecv", false) > 50)))
                {
                    Disconnected();
                    return(0);
                }
            }

//            // clear up
//            cCriticalSocket.Lock();
//            if (sSocket == SOCKET_ERROR)
//            {
//                Disconnected();
//                return(0);
//            };
                
//            FD_ZERO(&readsets);
        };
    }
    catch(...)
    {   // If we're not locked, no harm.  But if we are it prevents a deadlock.
        cCriticalSocket.Unlock();
        Disconnected();
        return(-1);
    }

    Disconnected();
    return(0);
};

// This thread waits for data

unsigned CConnectionSocket::ServiceConnectionThread(void* pVoid)
{
    CConnectionSocket *cConnectionSocket;
    unsigned uReturn;

    // Assign our "this" pointer
    if (!pVoid)
        return((unsigned) - 1);

    cConnectionSocket = (CConnectionSocket *) pVoid;
    uReturn = cConnectionSocket->HandleConnection();

    return(uReturn);
}


// Return the time the server was started

time_t CConnectionSocket::GetTimeConnected()
{
    return(tConnected);
};




// Connected?

BOOL CConnectionSocket::IsConnected()
{
    if ((int) sSocket == SOCKET_ERROR || !hThread)
        return(FALSE);

    return(CXPlatThread::IsThreadActive(hThread));
};


// Return the socket address

struct sockaddr_in CConnectionSocket::GetSocketAddress()
{
    return(sAddr);
};

