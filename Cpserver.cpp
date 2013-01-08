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
// CPServer.cpp: implementation of the CPServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XPlat.h"
#include "stldef.h"
#include "CXPlatThread.h"
#include "CBaseSocket.h"
#include "CPConnection.h"
#include "CPConnectionHandler.h"
#include "CPServer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPServer::CPServer(CP_CONNECTION_HANDLER_ARRAY &cPConnectionHandlers) : CBaseSocket()
{
    memset(&hThread, 0, sizeof(hThread));
    tStarted = time(NULL);
	cHandlers = cPConnectionHandlers;
	iNumHandlers = cHandlers.size();
}

CPServer::~CPServer()
{
	ShutdownAll();
}


    // Listen for someone trying to connect
BOOL CPServer::Listen(int nConnectionBacklog)
{       
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
		unsigned    uThread;
	   	hThread = (HANDLE) ::_beginthreadex(NULL, 
											0, 
											AcceptConnection, 
											(void *) this, 
											0,	// initflag = 0, start suspended
											&uThread);
        if (hThread)
			{
			::SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
            ::ResumeThread(hThread);
			};
        return(TRUE);
#else
		int		iReturn;
		iReturn	= ::pthread_create(	&hThread, 
									NULL,
									(START_ROUTINE) AcceptConnection,
									(void *) this);
		return(TRUE);
#endif
        };

#ifdef DEBUGGING
#if defined(_WIN32)
	iError = ::WSAGetLastError();
#else
	iError = errno;
#endif
	szError = ::strerror(iError);
#endif

    return(FALSE);
}



    // Accept the connection
BOOL CPServer::Accept()
{		// Accept a connection, and create a thread.
	SOCKET				hTemp;
    CPConnection		*pConnection;
    struct sockaddr     sAddr;
    socklen_t           iLen;
	fd_set				readsets;
	fd_set				exceptsets;
	int					nReturn;
	int					iLeastActive;
	int					iX;
	CPConnectionHandler	*pHandler;
	BOOL				bAddToHandler;
#if defined(__linux)	
	//	Because LinuxThreads are separate processes, doing a close
	//	on a socket in a thread doesn't unblock select or listen in another
	//  thread.  So for Linux, we'll timeout after 5 seconds and loop
	struct timeval		sTv;
	int					iTOSec = 5;
#endif
	
	while (TRUE)
		{   // Accept a connection
        iLen = sizeof(sAddr);
        // Clear sets
		FD_ZERO(&readsets);
		FD_ZERO(&exceptsets);;
			// Init sets
		cCriticalSocket.Lock();
			// Someone close it?
		if (sSocket == INVALID_SOCKET)
			{
			cCriticalSocket.Unlock();
			return(FALSE);
			};
		FD_SET(sSocket, &readsets);
		FD_SET(sSocket, &exceptsets);
		cCriticalSocket.Unlock();

#if defined(__linux)	
#ifdef KERNEL_2_4        
		sTv.tv_sec = iTOSec;
		sTv.tv_usec = 0;
		nReturn = ::select(sSocket + 1, &readsets, NULL, &exceptsets, &sTv);
#else
		nReturn = ::select(sSocket + 1, &readsets, NULL, &exceptsets, NULL);
#endif
#else
		nReturn = ::select(sSocket + 1, &readsets, NULL, &exceptsets, NULL);
#endif
		if (nReturn == SOCKET_ERROR)
			return(FALSE);

		cCriticalSocket.Lock();
			// Someone close it?
		if (sSocket == INVALID_SOCKET)
			{
			cCriticalSocket.Unlock();
			return(FALSE);
			};
		nReturn = FD_ISSET(sSocket, &readsets);
		cCriticalSocket.Unlock();

			// We have data
		if (!nReturn)
			continue;

		hTemp = ::accept(sSocket, &sAddr, &iLen);

		if (hTemp == INVALID_SOCKET)
			return(FALSE);

            // Make sure our socket isn't closed
            // If we're in our destructor, the derived class
            // is already destroyed and AllocateSocketClass
            // will be calling a pure virtual function.
        if (sSocket == INVALID_SOCKET)
            return(FALSE);
        
            // Create a socket class to service this connection
		try
			{
			pConnection = AllocateSocketClass(&sAddr);
			}
		catch(...)
			{
			pConnection = NULL;
			};

		if (pConnection == NULL)
			{	// Clean up array.
			CleanupArray();
			continue;
			};

			// Good connection
		pConnection->eConnected = CPConnection::CONNECT_SUCCESS;

            // Pass to a connection handler
		if (iNumHandlers)
			{
			pHandler = (CPConnectionHandler *) cHandlers[0];
			iLeastActive = pHandler->GetLastSize();
			for (iX = 1; iX < iNumHandlers; iX++)
				{
				if (cHandlers[iX]->GetCurrentSize() < iLeastActive)
					pHandler = (CPConnectionHandler *) cHandlers[iX];
				};
			bAddToHandler = pHandler->AddConnection(pConnection, hTemp);
			}
		else				
			{
			bAddToHandler = FALSE;
			};

			// Successfully added to handler
		if (!bAddToHandler)
			{
			pConnection->eConnected = CPConnection::CONNECT_DISCONNECTED;
			pConnection->TerminateConnection();
			continue;
			};

            // Clean up array.
        CleanupArray();

            // Keep track of objects in an array here and then
            // terminate each for during shutdown
		cCritSocketArray.Lock();
#ifndef _MFC_VER
        cSocketArray.push_back(pConnection);
#else
        cSocketArray.Add(pConnection);
#endif            
		cCritSocketArray.Unlock();

		hTemp = INVALID_SOCKET;	// Reset
		};
	return TRUE;
}

    
    // Clean up array here.  Delete any objects without a
    // thread which is currently running.
void CPServer::CleanupArray(BOOL bTerminateAll)
{        
    vector<CPConnection *>::iterator    Iter;
    CPConnection						*cSocket;

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
        cSocket = (CPConnection *) *Iter;
            // If we're terminating all, just delete the cSocket pointer
        if (bTerminateAll)
            delete cSocket;	// Destructor will remove socket from the handler
		else
			{	// Socket disconnected? If so, mark for removal by handler.
			if (cSocket->GetConnectionStatus() >= CPConnection::CONNECT_DISCONNECTED)
				{
				delete cSocket;	// Destructor will remove socket from the handler
				Iter = cSocketArray.erase(Iter);	// Remove from our array
				continue;
				};
			};
		Iter++;
        };

        // If we're terminating all, we're done
    if (bTerminateAll)
	    cSocketArray.clear();

	cCritSocketArray.Unlock();
};

            
    // Kill all connections
void CPServer::ShutdownAll()
{
    CloseSocket();	// Close listener socket
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
	CleanupArray(TRUE);
};


    // Thread to accept connections
unsigned CPServer::AcceptConnection(void* pVoid)
{
	unsigned		uReturn;

	uReturn = ((CPServer *) pVoid)->Accept();	

	return(uReturn);;
};


    // Return the time the server was started
time_t CPServer::GetTimeStarted()
{
    return(tStarted);
};




