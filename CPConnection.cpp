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
// CPConnection.cpp: implementation of the CPConnection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XPlat.h"
#include "CBaseSocket.h"
#include "CPConnection.h"
#include "CPConnectionHandler.h"

    // Constructor
CPConnection::CPConnection(	CBaseSocket *pOwner, 
							struct sockaddr *pAddr,  
							int iMaxSendBufferSize) : CBaseSocket()
{
    cOwner = pOwner;
    memset(&sAddr, 0, sizeof(sAddr));
    if (pAddr)
        memcpy(&sAddr, pAddr, sizeof(sAddr));
	pOutBuffer = NULL;					
	lCloseRequestTS = iOBSize = iLeftToSend = iSendIndex = 0;
	iMaxSendBuffer = iMaxSendBufferSize;
	bSendData = bSendBufferHighWaterMark = bRecvPause = FALSE;
	cPHandler = NULL;
	eConnected = CONNECT_UNUSED;
	cSignature.pConnection = this;
	cSignature.ulTimeStamp = ::XPlatGetMilliseconds();
	iTotalRecvd = iTotalSent = iTotalSendData = 0;
};


	// Only a CPConnectionHandler should delete a CPConnection
CPConnection::~CPConnection()
{		
		// Remove from handler. RemoveConnection sets our cPHandler ptr to NULL
	if (cPHandler)
		{
		cPHandler->RemoveConnection(this);
		cPHandler = NULL;
		};
		// Delete the output buffer
	cCriticalSocket.Lock();
	UnprotectedCloseSocket();
	if (pOutBuffer)
		{
		delete[] pOutBuffer;
		pOutBuffer = NULL;
		};
	cCriticalSocket.Unlock();	
}

    // Create our internal socket
BOOL CPConnection::Create(	int af, 
							int type, 
							int protocol, 
							UINT nPort, 
							LPCSTR lpAddr, 
							BOOL bReuse, 
							BOOL bBind, 
							int iLinger)
{		
	BOOL bResult;

		// For all CPConnection sockets, use non-blocking mode
	if ((bResult = CBaseSocket::Create(af, type, protocol, nPort, lpAddr, bReuse, bBind, iLinger)) == TRUE)
		{
		if ((bResult = SetBlockingMode(FALSE)) == FALSE)
			CloseSocket();
		};

	return(bResult);
};


	// Store pointer when a handler is chosen to handle this connection
void CPConnection::SetHandler(CPHandler *pHandler)
{
	cPHandler = (CPConnectionHandler *) pHandler;
		// Let the object do any prep work
	if (pHandler)
		pHandler->CallHandleConnection(this);
};

    // When called, this removes the socket from the connection handler
void CPConnection::TerminateConnection()
{
		// Now close the socket.  Calls cCriticalSocket.Lock().  For this reason,
		// we are assured that it will be removed from any connection handler before
		// the next I/O loop starts.
    CloseSocket();	
		// NOTE:	You could call cCriticalSocket.Lock here and delete the pOutBuffer.
		//			But that will be done during cleanup up without any synchronization issues.
	eConnected = CONNECT_DISCONNECTED;
};


	// Put data in output queue to prepare for sending.
	// If iMaxSendBufferSize is exceeded, the return will be FALSE,
	// and the sender MUST WAIT until some data is sent before proceeding!
	// nPacketSize is ignored, and is just included for backward
	// compatibility with CConnectionSocket.
BOOL CPConnection::SendDataOut(LPSTR lpData, int nPacketSize, int nTotalLength)
{	
	int				iNewBufSize;
	unsigned char	*pWork = NULL;
		
		// Make sure pointer isn't NULL
	if (!lpData || !nTotalLength)
		return FALSE;

	cCriticalSocket.Lock();

	if (sSocket == INVALID_SOCKET)
		{
		cCriticalSocket.Unlock();
		return(FALSE);
		};

	iTotalSendData += nTotalLength;

		// Check size of buffer.  Dynamically adjust buffer size.
	iNewBufSize = iOBSize;
	while (iNewBufSize < (nTotalLength + iLeftToSend))
		iNewBufSize += 512;
	if (iNewBufSize < 1024)
		iNewBufSize = 1024;	// Minimum start buffer size

		// If we have to expand the buffer, do it now
	if (iNewBufSize > iOBSize)
		{
		pWork = new unsigned char[iNewBufSize];
		assert(pWork != NULL);

			// Copy what remains in the old buffer to send into the new buffer
		if (iLeftToSend)
			::memcpy(pWork, pOutBuffer + iSendIndex, iLeftToSend);

			// Copy new stuff at the end
		::memcpy(pWork + iLeftToSend, lpData, nTotalLength);			

			// Get rid of old buffer
		if (pOutBuffer)
			delete[] pOutBuffer;

			// Assign new one
		pOutBuffer = pWork;
		pWork = NULL;

			// Adjust all stuff
		iOBSize = iNewBufSize;
		}
	else
		{	// Move old data to the front
		if (iSendIndex)
			::memmove(pOutBuffer, pOutBuffer + iSendIndex, iLeftToSend);
			// Add new data to the end
		::memcpy(pOutBuffer + iLeftToSend, lpData, nTotalLength);
		};
		
	iSendIndex = 0;
	iLeftToSend += nTotalLength;

	bSendData = iLeftToSend > 0 ? TRUE : FALSE;

	if (iLeftToSend + CS_BUFFER_SIZE >= iMaxSendBuffer)
		bSendBufferHighWaterMark = TRUE;
	else
		bSendBufferHighWaterMark = FALSE;

	cCriticalSocket.Unlock();
	
	return(TRUE);
}



	// Get connection status
CPConnection::ECONNECT CPConnection::GetConnectionStatus()
{
	return(eConnected);
};


    // Return the time the server was started
time_t CPConnection::GetTimeConnected()
{
    return(tConnected);
};


	// Return the socket address
struct sockaddr_in CPConnection::GetSocketAddress()
{
	return(sAddr);
};
	

	// Send data from a socket
int CPConnection::SendData()
{
	int				iReturn = 0;
	int				iAmtToSend;

		// Don't need cCriticalSocket.Lock() - done in CPConnectionHandler

		// Don't send more than 32K at a time
	iAmtToSend = min((int)CS_OUTBUFFER_SIZE, iLeftToSend);

	try {
		iReturn = ::send(	sSocket, 
							(LPSTR) pOutBuffer + iSendIndex, 
							iAmtToSend, 
							0);
		}
	catch(...) 
		{
		return(SOCKET_ERROR);
		}
	if (iReturn == SOCKET_ERROR)
		{
#if defined(_WIN32)
	    iReturn = ::WSAGetLastError();
#else
		iReturn = errno;
#endif
		if (iReturn != XEWOULDBLOCK && iReturn != XEINPROGRESS)
			return(SOCKET_ERROR);
		iReturn = 0;	// XEWOULDBLOCK is 10035, we don't want to say we sent that!
		};

	iTotalSent += iReturn;

	iLeftToSend -= iReturn;

	iSendIndex += iReturn;

	bSendData = iLeftToSend > 0 ? TRUE : FALSE;

	if (iLeftToSend + CS_BUFFER_SIZE < iMaxSendBuffer)
		bSendBufferHighWaterMark = FALSE;

		// Last data sent.  Update "Close connection" timestamp
	if (!bSendData && eConnected == CONNECT_CLOSE_REQUESTED)
		lCloseRequestTS = ::XPlatGetMilliseconds();

	return(iReturn);
}



	// Receive data to a socket
int CPConnection::ReceiveData()
{
	int		nRecvLength;
	int		iReturn = 0;

	if (bRecvPause)
		return(0);

		// Don't need cCriticalSocket.Lock() - done in CPConnectionHandler
	try
		{
		nRecvLength = ::recv(sSocket, 
							(LPSTR) ucBuffer, 
							sizeof(ucBuffer), 
							0);
		}
	catch(...)
		{
		return(SOCKET_ERROR);
		};		
	if(nRecvLength == SOCKET_ERROR || nRecvLength == 0)
		{
#if defined(_WIN32)
		iReturn = ::WSAGetLastError();
#else
		iReturn = errno;
#endif
		if (iReturn != XEWOULDBLOCK && iReturn != XEINPROGRESS || !nRecvLength)
			return(SOCKET_ERROR);
		return(0);
		};
	if(nRecvLength > (int) sizeof(ucBuffer))
		return(SOCKET_ERROR);
	if (nRecvLength > 0)
		{
		iTotalRecvd += nRecvLength;
		ProcessData(ucBuffer, nRecvLength);
		};

	return(0);
};


	// Asynchronously close the socket.  If a handler hasn't
	// been assigned, this calls CloseSocket.
	// Otherwise, the handler waits until the send buffer is
	// clear and then closes the socket.
void CPConnection::AsyncTerminateConnection()
{		// No handler
	if (!cPHandler)
		{
		TerminateConnection();
		return;
		};

		// Signal that we'd like the handler to terminate after sends are complete
	eConnected = CONNECT_CLOSE_REQUESTED;
	lCloseRequestTS = ::XPlatGetMilliseconds();
};


    // Unprotected close socket
void CPConnection::UnprotectedCloseSocket()
{   
	try
		{	// If there is an exception, it's a race condition, which
			// in this case means someone else is closing it.  No Problem.
			// It gets accomplished.
		eConnected = CONNECT_DISCONNECTED;
		CBaseSocket::UnprotectedCloseSocket();
		}
	catch(...)
		{
		};
};
