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
//
//	Created: Richard A. Bross
//
#if defined(_WIN32)
#include "stdafx.h"
#endif
#include "XPlat.h"
#include "CBaseSocket.h"
#include "CConnectionSocket.h"
#include "CServerSocket.h"

// constructor

CServerSocket::CServerSocket() : CBaseSocket()
{
    memset(&hThread, 0, sizeof(hThread));
    tStarted = time(NULL);
}

CServerSocket::~CServerSocket()
{
    ShutdownAll();
}



// Listen for someone trying to connect

BOOL CServerSocket::Listen(int nConnectionBacklog)
{
#ifdef DEBUGGING
    int iError;
    char *szError;
#endif

    // Already listening?
    if (hThread)
    {
        if (CXPlatThread::IsThreadActive(hThread))
            return(TRUE);
        else
        {
#if defined(_WIN32)
            ::CloseHandle(hThread);
#endif
            memset(&hThread, 0, sizeof(hThread));
        };
    };

    // Listen and start thread
    if (SOCKET_ERROR != ::listen(sSocket, nConnectionBacklog))
    {
#if defined(_WIN32)
        unsigned uThread;
        hThread = (HANDLE) ::_beginthreadex(NULL,
            0,
            AcceptConnection,
            (void *) this,
            0, // initflag = 0, start suspended
            &uThread);
        if (hThread)
            ::ResumeThread(hThread);
        return(TRUE);
#else
        int iReturn;
        iReturn = ::pthread_create(&hThread, pthread_attr_default, (START_ROUTINE) AcceptConnection, (void *) this);

        return(TRUE);
#endif
    };

#ifdef DEBUGGING
    iError = XPLAT_WSAGETLASTERR;
    szError = ::strerror(iError);
#endif

    return(FALSE);
}



// Accept the connection

BOOL CServerSocket::Accept()
{ // Accept a connection, and create a thread.
    SOCKET hTemp;
    CConnectionSocket *pConnection;
    struct sockaddr sAddr;
    socklen_t iLen;
    fd_set readsets;
    int nReturn;
#if defined(__linux)	
    //	Because Linux select doesn't automatically unblock, we'll timeout after 1 second and loop
    struct timeval sTv;
    long iTOSec = 1;
#endif
    time_t tLastCleanup = ::time(NULL);

    while(TRUE)
    { // Accept a connection
        iLen = sizeof(sAddr);
        // Clear sets
        FD_ZERO(&readsets);
        ;
        // Init sets
        cCriticalSocket.Lock();
        // Someone close it?
        if (sSocket == SOCKET_ERROR)
        {
            cCriticalSocket.Unlock();
            return(FALSE);
        };
        FD_SET(sSocket, &readsets);
        cCriticalSocket.Unlock();

#if defined(__linux)
        sTv.tv_sec = iTOSec;
        sTv.tv_usec = 0;
        try
        {
            nReturn = ::select(sSocket + 1, &readsets, NULL, NULL, &sTv);
        }
        catch(...)
        {
            nReturn = SOCKET_ERROR;
        }
#else
        nReturn = ::select(sSocket + 1, (XPLAT_FD_SET_PTR) & readsets, NULL, (XPLAT_FD_SET_PTR) & exceptsets, NULL);
#endif
        if (nReturn == SOCKET_ERROR)
            return(FALSE);

        cCriticalSocket.Lock();
        // Someone close it?
        if (sSocket == SOCKET_ERROR)
        {
            cCriticalSocket.Unlock();
            return(FALSE);
        };
        nReturn = FD_ISSET(sSocket, &readsets);
        cCriticalSocket.Unlock();

        // We have data
        if (!nReturn)
            continue;

#if defined(__linux)
        try
        {
#endif
            hTemp = ::accept(sSocket, &sAddr, &iLen);
#if defined(__linux)
        }
        catch(...)
        {
            hTemp = SOCKET_ERROR;
        }
#endif

        if (SOCKET_ERROR == hTemp)
            return(FALSE);

        // Make sure our socket isn't closed
        // If we're in our destructor, the derived class
        // is already destroyed and AllocateSocketClass
        // will be calling a pure virtual function.
        if (sSocket == SOCKET_ERROR)
            return(FALSE);

        // Create a socket class to service this connection
        
        pConnection = AllocateSocketClass(&sAddr);

        if (pConnection == NULL)
        { // Clean up array.
            CleanupArray();
            continue;
        };

        // Keep track of objects in an array here and then terminate each during shutdown
        // *** RAB - 4/12/2012 - for years this was after the ServiceConnection call.
        //                       that would seem like a problem since it could finish prior to being inserted.
        cCritSocketArray.Lock();
        cSocketArray.push_back(pConnection);
        cCritSocketArray.Unlock();

        // Start the socket servicing the client
        pConnection->ServiceConnection(hTemp);

        // Clean up array.
        CleanupArray();

        hTemp = SOCKET_ERROR; // Reset
    };
    return TRUE;
}


// Clean up array here.  Delete any objects without a
// thread which is currently running.

void CServerSocket::CleanupArray(BOOL bTerminateAll)
{
    vector<void *>::iterator Iter;
    CConnectionSocket *cSocket;

    cCritSocketArray.Lock();

    // Anything in array?
    if (!cSocketArray.size())
    {
        cCritSocketArray.Unlock();
        return;
    };

    // Load the temp array with active objects, delete the rest
    for(Iter = cSocketArray.begin(); Iter != cSocketArray.end();)
    {
        cSocket = (CConnectionSocket *) *Iter;
        if (bTerminateAll)
        {
            delete cSocket;
        }
        else
        { // Thread terminated? If so, clean up.
            if (!CXPlatThread::IsThreadActive(cSocket->hThread))
            {
                delete cSocket;
                Iter = cSocketArray.erase(Iter);
                continue;
            };
        }
        Iter++;
    };

    // If we're terminating all, we're done
    if (bTerminateAll)
        cSocketArray.clear();

    cCritSocketArray.Unlock();
};


// Kill all connections

void CServerSocket::ShutdownAll()
{
    CleanupArray(TRUE);
    CloseSocket();
    if (hThread)
    {
#if defined(_WIN32)
        ::WaitForSingleObject(hThread, INFINITE);
        ::CloseHandle(hThread);
#else
        ::pthread_join(hThread, NULL);
#endif
        memset(&hThread, 0, sizeof(hThread));
    };
};


// Thread to accept connections

unsigned CServerSocket::AcceptConnection(void* pVoid)
{
    unsigned uReturn;

    uReturn = ((CServerSocket *) pVoid)->Accept();

    return(uReturn);
};


// Return the time the server was started

time_t CServerSocket::GetTimeStarted()
{
    return(tStarted);
};


// Wait for listening thread to exit

void CServerSocket::WaitForExit()
{
    if (hThread)
    {
#if defined(_WIN32)
        ::WaitForSingleObject(hThread, INFINITE);
        ::CloseHandle(hThread);
#else
        ::pthread_join(hThread, NULL);
#endif
        memset(&hThread, 0, sizeof(hThread));
    };
}


