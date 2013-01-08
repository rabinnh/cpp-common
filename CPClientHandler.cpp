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
// CPClientHandler.cpp: implementation of the CPClientHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CPHandler.h"
#include "CPClientHandler.h"
#include "CPConnection.h"
#include "CPClient.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPClientHandler::CPClientHandler() : CPHandler()
{
	StartThread();
}

	// This object should not be deleted until all CPClient objects
	// have been removed by calls to RemoveConnection.
	// If that has occurred, then there should not be a thread running,
	// because the thread will end when the last object has been removed.
CPClientHandler::~CPClientHandler()
{
};


	// Thread that actually handles the connection attempt
unsigned CPClientHandler::WorkerFunction(void *pParam)
{
	fd_set				exceptsets;
	fd_set				writesets;
	int					nReturn;
	CONNECTION_ITERATOR	IterSocket;
	CONNECTION_ITERATOR	IterSocket2;
	CPClient			*pClient;
	SOCKET				sHighSocket;
	struct timeval		sTv;
	int					iTOusec = 500;	// .5ms (500 microsecond) delay if the server does respond immediately
	unsigned long		ulCurrentMS;
	BOOL				bAttemptComplete;
	BOOL				bSuccess;
	BOOL				bErased;

	while(TRUE)
		{   
		if (!bThreadRunning)
			return(0);	// They want us to quit

			// Clear sets
		FD_ZERO(&exceptsets);	// Connection failed
		FD_ZERO(&writesets);	// Connection succeeded

			// This will have to remain locked for most of
			// the cycle.  Otherwise, a caller could delete a CPConnection
			// object while we're trying to use it.
		cCritMap.Lock();	
			// Add all queued connections
		cCritQueuedMap.Lock();
		IterSocket = cQueuedConnections.begin();
		while(IterSocket != cQueuedConnections.end())
			{
			pClient = (CPClient *) IterSocket->second;
			if (pClient->eConnected == CPClient::CONNECT_PENDING)
				cLiveConnections.insert(CONNECTION_MAP::value_type(pClient->cSignature, pClient));
			IterSocket++;
			};
		cQueuedConnections.clear();
		iSize = iLiveSize = cLiveConnections.size();
		cCritQueuedMap.Unlock();

		    // Iterate through map
		sHighSocket = -1;
		IterSocket = cLiveConnections.begin();
		while(IterSocket != cLiveConnections.end())
			{
			pClient = (CPClient *) IterSocket->second;
				// Defensive coding, this shouldn't happen.
			if (pClient->sSocket == INVALID_SOCKET || pClient->eConnected != CPClient::CONNECT_PENDING)
				{	// This will return the next iterator in the map
				IterSocket2 = IterSocket;	
				IterSocket2++;
				cLiveConnections.erase(IterSocket);	
				IterSocket = IterSocket2;
				iSize--;
				continue;
				};
			FD_SET(pClient->sSocket, &exceptsets);
			FD_SET(pClient->sSocket, &writesets);
			sHighSocket = max((long) pClient->sSocket, (long) sHighSocket);
			IterSocket++;
			};

		if (sHighSocket == -1)
			{
			cCritMap.Unlock();
			Sleep(iTOusec);	
			continue;
			}
		else	// *** RAB
				// *** Try using connect calls instead of select for less use of CPU?
			{	// Blocking call
			sTv.tv_sec = 0;
			sTv.tv_usec = iTOusec;	// This prevents a tight while loop causing 100% CPU use
			try {
    			nReturn = ::select(sHighSocket + 1, NULL, &writesets, &exceptsets, &sTv);
				}
			catch(...) 
				{
				nReturn = SOCKET_ERROR;
				}
			};
 		
			// Some weird error or no sockets ready.
			// Unlike the connection handler, there is not work to do here if no
			// sockets are ready to be connected.
		if (nReturn == SOCKET_ERROR || !nReturn)
            {
			cCritMap.Unlock();	
			continue;
            };

			// Handle excepts, and writes for all sockets involved.
			// A write = a successful connect, an except = a failure

			// Get our time comparison for this cycle
		ulCurrentMS = ::XPlatGetMilliseconds();
		    // Iterate through map
		IterSocket = cLiveConnections.begin();
		while(IterSocket != cLiveConnections.end())
			{
			if (!bThreadRunning)
				return(0);	// They want us to quit
			try	
				{	
				bSuccess = bAttemptComplete = FALSE;
				pClient = (CPClient *) IterSocket->second;
				pClient->cCriticalSocket.Lock();
					// Defensive coding, this shouldn't happen.
				if (pClient->sSocket == INVALID_SOCKET || pClient->eConnected != CPConnection::CONNECT_PENDING)
					throw(0);

					// See if we connect was successful
				nReturn = FD_ISSET(pClient->sSocket, &writesets);
				if (nReturn)	// Connected
					bAttemptComplete = bSuccess = TRUE;
				else
					{	// See if connect timed out (default socket timeout)
					nReturn = FD_ISSET(pClient->sSocket, &exceptsets);
					if (nReturn)	// Couldn't connect
						bAttemptComplete = TRUE;
					else
						{	// See if our timeout was exceeded
						if (pClient->ulTimeout != INFINITE && 
							((ulCurrentMS - pClient->ulAttemptTS) > pClient->ulTimeout)) 
								bAttemptComplete = TRUE;
						};
					};

					// Callback the client. We have to remove the client from
					// our cLiveConnection map because our work is done and
					// the client now has to pass itself to a CPConnectionHandler.
				if (bAttemptComplete)
					{	// Set the value
					pClient->eConnected = bSuccess ? 
								CPConnection::CONNECT_SUCCESS : CPConnection::CONNECT_ATTEMPT_FAILED; 
						// Either way, we're done, so remove it.
					IterSocket2 = IterSocket;	
					IterSocket2++;
					cLiveConnections.erase(IterSocket);	
					IterSocket = IterSocket2;
					pClient->SetHandler(NULL);
					iSize--;
					pClient->cCriticalSocket.Unlock();
					if (pClient->pCPLFunc)
						pClient->ConnectCallback(pClient, bSuccess);
					continue;	// Client will remove from the cLiveConnection map
					};

				pClient->cCriticalSocket.Unlock();
				IterSocket++;
				}	// Try
			catch(...)	
				{	// In case "erase" was done before the callback
				bErased = FALSE;
				try
					{
					if (IterSocket != cLiveConnections.end())
						{
#if defined(_WIN32)
						pClient->iConnectError = ::WSAGetLastError();
#else
						pClient->iConnectError = errno;
#endif
						pClient = (CPClient *) IterSocket->second;
						IterSocket2 = IterSocket;	
						IterSocket2++;
						cLiveConnections.erase(IterSocket);	
						IterSocket = IterSocket2;
						iSize--;
						bErased = TRUE;
						pClient->eConnected = CPConnection::CONNECT_ATTEMPT_FAILED;
						if (pClient->pCPLFunc)
							pClient->ConnectCallback(pClient, FALSE);
						else
					        pClient->UnprotectedCloseSocket();
						pClient->cCriticalSocket.Unlock();	// Unlock
						};
					}
				catch(...)
					{	// Start clean
					if (!bErased)
						{
						IterSocket2 = IterSocket;	
						IterSocket2++;
						cLiveConnections.erase(IterSocket);	
						IterSocket = IterSocket2;
						iSize--;
						};
					break;
					};
				}
			}	// while iterator
		cCritMap.Unlock();
		};	// while(TRUE)

	bThreadRunning = FALSE;

	return(0);
};		


