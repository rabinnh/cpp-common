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
// CPHandler.cpp: implementation of the CPHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CPHandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

	// Not much to do here
CPHandler::CPHandler()
{
	iSize = iLiveSize = 0;
	bThreadRunning = TRUE;
}

	// This object should not be deleted until all CPConnection objects
	// have been removed by calls to RemoveConnection.
	// If that has occurred, then there should not be a thread running,
	// because the thread will end when the last object has been removed.
CPHandler::~CPHandler()
{		
	bThreadRunning = FALSE;
	WaitForThreadCompletion();
}


	// Add a connection to the queued.
	// These are added to cQueuedConnections.
	// At the beginning of the next I/O cycle, they are added to cLiveConnections.
BOOL CPHandler::AddConnection(CPConnection *pConnection, SOCKET sNewSocket)
{
	CONNECTION_ITERATOR	IterSocket;
	unsigned char		*ucHS;
	int					nReturn;

		// Check pointer
	if (!pConnection)
		return(FALSE);

		// If not connected, forget it
	if (pConnection->eConnected != CPConnection::CONNECT_SUCCESS
						&& pConnection->eConnected != CPConnection::CONNECT_PENDING)
		return(FALSE);

	cCritQueuedMap.Lock();

		// Assign the socket
	if (sNewSocket != INVALID_SOCKET)
		{
		pConnection->sSocket = sNewSocket;
		pConnection->SetBlockingMode(FALSE);
		};

		// Timestamp.
	pConnection->tConnected = time(NULL);

        // See if it's already there
    IterSocket = cQueuedConnections.find(pConnection->cSignature);

        // Found?
    if (IterSocket != cQueuedConnections.end())
		{	// This socket is already in the map
		cCritQueuedMap.Unlock();
		return(TRUE);	// Although technically an error, for calling it twice, it IS in the map.
		}
	else
		{	// Add
		cQueuedConnections.insert(CONNECTION_MAP::value_type(pConnection->cSignature, pConnection));
		pConnection->SetHandler(this);
		if (pConnection->GetHandshake(&ucHS, nReturn))
			pConnection->SendDataOut((char *) ucHS, 4096, nReturn);
		iSize++;
		};

	cCritQueuedMap.Unlock();

	return(TRUE);
};



	// Remove a connection from cLiveConnections.
	// This will wait until the next I/O cycle is complete.
	// This is usually only called from a CPConnection
	// destructor, since it has to wait to be removed before
	// being deleted.
BOOL CPHandler::RemoveConnection(CPConnection *pConnection)
{
	CONNECTION_ITERATOR	IterSocket;
	CONNECTION_ITERATOR	IterSocket2;

		// If it's not connected, forget it.
	if (!pConnection)
		return(FALSE);

		// First see if it's in the queue map
	cCritQueuedMap.Lock();

        // See if it's already there
    IterSocket = cQueuedConnections.find(pConnection->cSignature);
        // Found?
    if (IterSocket != cQueuedConnections.end())
		{
		IterSocket2 = IterSocket;	
		IterSocket2++;
		cQueuedConnections.erase(IterSocket);	
		IterSocket = IterSocket2;
		pConnection->SetHandler(NULL);
		iSize--;
		}

	cCritQueuedMap.Unlock();

	cCritMap.Lock();

        // See if it's already there
    IterSocket = cLiveConnections.find(pConnection->cSignature);
        // Found?
    if (IterSocket != cLiveConnections.end())
		{
		IterSocket2 = IterSocket;	
		IterSocket2++;
		cLiveConnections.erase(IterSocket);	
		IterSocket = IterSocket2;
		pConnection->SetHandler(NULL);
		iSize--;
		}
	else
		{	// Not found, which was the point, I guess.
		}

	cCritMap.Unlock();

		// Call the sockets Disconnected member.
	CallDisconnected(pConnection);

	return(TRUE);
};




	// Called in the case of an unrecoverable error
void CPHandler::DisconnectAll()
{ 
	CPConnection		*pConnection;
	CONNECTION_ITERATOR	IterSocket;

	    // Iterate through map
	for (IterSocket = cLiveConnections.begin(); IterSocket != cLiveConnections.end(); IterSocket++)
		{
		pConnection = IterSocket->second;
			// Defensive coding, this shouldn't happen.
		pConnection->CloseSocket(); 
		pConnection->Disconnected();
		};

	cLiveConnections.clear();
};



	// Get the last size (not necesssarily current)
int	CPHandler::GetLastSize()
{
	return(iSize);
};


	// Get the current size (will wait for critical section)
int	CPHandler::GetCurrentSize()
{
	return(iLiveSize);
};


	// Call the cPConnection->HandleConnection
	// This is here so derived classes can choose NOT to call it
	// in case the handler is chained (like the CPClientHandler)
int	CPHandler::CallHandleConnection(CPConnection *pConnection)
{
	if (pConnection)
		return(pConnection->HandleConnection());
	else
		return(0);
};


	// If we want to call the sockets Disconnected member
void CPHandler::CallDisconnected(CPConnection *pConnection)
{
	pConnection->Disconnected();
};
