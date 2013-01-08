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
// CPConnectionHandler.cpp: implementation of the CPConnectionHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CPHandler.h"
#include "CPConnectionHandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

	// Not much to do here
CPConnectionHandler::CPConnectionHandler(long lCloseWaitMilliseconds) : CPHandler()
{
	lCloseWaitMS = lCloseWaitMilliseconds;
	StartThread();
}

	// This object should not be deleted until all CPConnection objects
	// have been removed by calls to RemoveConnection.
	// If that has occurred, then there should not be a thread running,
	// because the thread will end when the last object has been removed.
CPConnectionHandler::~CPConnectionHandler()
{	
}



	// Thread that actually handles the connections
unsigned CPConnectionHandler::WorkerFunction(void *pParam)
{
	CONNECTION_ITERATOR	IterSocket;
	CONNECTION_ITERATOR	IterSocket2;
	CPConnection		*pConnection;
	BOOL				bInvalidateSocket;
	long				lNow;

	while(TRUE)
		{   
		if (!bThreadRunning)
			return(0);	// They want us to quit
		
			// This will have to remain locked for most of this processing
			// cycle.  Otherwise, a caller could delete a CPConnection
			// object while we're trying to use it.
		cCritMap.Lock();	

			// Add all queued connections
		cCritQueuedMap.Lock();
		IterSocket = cQueuedConnections.begin();
		while(IterSocket != cQueuedConnections.end())
			{
			try
				{	// There is a slight chance it's getting deleted while it's still
					// in the cQueuedConnections map.
				pConnection = IterSocket->second;
// *** RAB - should save the queued connection to the pointer, delete it from the queued connections and then delete it.
//			 It shouldn't live in both at once.  This would also make the code more granular, as it would free the data
//			 structures in between removing from one map and placing it in another.
			        // Found?
				cLiveConnections.insert(CONNECTION_MAP::value_type(pConnection->cSignature, pConnection));
				}
			catch(...)
				{
				};
			IterSocket++;
			};
		cQueuedConnections.clear();
		iSize = iLiveSize = cLiveConnections.size();
		cCritQueuedMap.Unlock();

			// Temporarily free it to allow other thread activity
		cCritMap.Unlock();	

			// Lock it again
		cCritMap.Lock();	

		iLiveSize = cLiveConnections.size();
		IterSocket = cLiveConnections.begin();
		while(IterSocket != cLiveConnections.end())
			{
			pConnection = IterSocket->second;
				// Defensive coding, this shouldn't happen.
			if (pConnection->sSocket == INVALID_SOCKET || 
					pConnection->eConnected >= CPConnection::CONNECT_DISCONNECTED)
				{	// This will return the next iterator in the map
				IterSocket2 = IterSocket;	
				IterSocket2++;
				cLiveConnections.erase(IterSocket);	
				IterSocket = IterSocket2;				
				iSize--;
				continue;
				};
			IterSocket++;
			};
		
        Sleep(1);		// Under Windows, this will yield the remainder of this
						// thread's timeslice.

		if (!iSize)
			{
			cCritMap.Unlock();	
			continue;
			};
   	
			// Get the current time
		lNow = ::XPlatGetMilliseconds();

			// Handle writes, and reads for all sockets involved.
			// If the select returned 0, meaning no sockets are ready to be read,
			// we must still check to see if there is anything that is supposed
			// to be sent.

		    // Iterate through map
		IterSocket = cLiveConnections.begin();
		while(IterSocket != cLiveConnections.end())
			{
			if (!bThreadRunning)
				return(0);	// They want us to quit
			try	
				{	
				pConnection = IterSocket->second;
				pConnection->cCriticalSocket.Lock();
				bInvalidateSocket = FALSE;

					// Defensive coding.  Someone could have closed the socket.
				if (pConnection->sSocket == INVALID_SOCKET)
					bInvalidateSocket = TRUE;
	
					// See if we need to do a read
					// Don't read if we're waiting to close.
				if (!bInvalidateSocket && pConnection->eConnected == CPConnection::CONNECT_SUCCESS)
					{
	   				if (pConnection->ReceiveData() == SOCKET_ERROR)
						bInvalidateSocket = TRUE;
					};
	
					// Defensive coding.  Someone could have closed the socket.
				if (pConnection->sSocket == INVALID_SOCKET)
					bInvalidateSocket = TRUE;
	
					// See if we need to do a send
				if (!bInvalidateSocket)
					{
					if (pConnection->bSendData)	// We have data
						{
		   				if (pConnection->SendData() == SOCKET_ERROR)
							bInvalidateSocket = TRUE;
						}
					else
						{	// If all data sent
						if (pConnection->eConnected == CPConnection::CONNECT_CLOSE_REQUESTED)
							{	// Time elapsed?
							if (!pConnection->lCloseRequestTS || 
									GetTimeDiff(pConnection->lCloseRequestTS, lNow) >= lCloseWaitMS)
								bInvalidateSocket = TRUE;
							};
						};
					};
				}	// Try
			catch(...)	
				{
				bInvalidateSocket = TRUE;
				};

			if (bInvalidateSocket)
				{
				iSize--;
				iLiveSize--;
				pConnection->SetHandler(NULL);
				IterSocket2 = IterSocket;	
				IterSocket2++;
				cLiveConnections.erase(IterSocket);	
				IterSocket = IterSocket2;				
				CallDisconnected(pConnection);	// Call the disconnect
				pConnection->UnprotectedCloseSocket();	// Really close it.  We already are protected.
				pConnection->cCriticalSocket.Unlock();	// Unlock
				continue;
				};
    
			pConnection->cCriticalSocket.Unlock();
			try
				{
				IterSocket++;
				}
			catch(...)
				{	// This shouldn't happen.  We'll just start with a new group.
				break;	
				};
			}	// while iterator
		cCritMap.Unlock();
		};	// while(TRUE)

	bThreadRunning = FALSE;
	return(0);
};		



	// Get MS difference between 2 times
long CPConnectionHandler::GetTimeDiff(long lStart, long lFinish)
{	
	long		lElapsed;

		// Get the total response time
	lElapsed = lFinish - lStart;
	if (lElapsed < 0)
		lElapsed += ((DWORD) -1) >> 1;

	return(lElapsed);
};

