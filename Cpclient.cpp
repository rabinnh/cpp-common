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
// CPClient.cpp: implementation of the CPClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CPClient.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPClient::CPClient(	CP_CONNECTION_HANDLER_ARRAY &cPCHandlers, 
					CP_CLIENT_HANDLER_ARRAY &cPClientHandlers, 
					int iMaxSendBufferSize) : CPConnection(NULL, NULL, iMaxSendBufferSize)
{
	cConnectionHandlers = cPCHandlers;
	cClientHandlers = cPClientHandlers;
	iNumConnHandlers = cConnectionHandlers.size();
	iNumClientHandlers = cClientHandlers.size();
	pCPLFunc = NULL;
	iConnectError = 0;
}

CPClient::~CPClient()
{

}

    // Connect to a server
BOOL CPClient::ConnectToServer(LPCSTR lpszHostAddress, UINT nHostPort, DWORD dwTimeout, LPCSTR lpszClientAddress)
{	
		// Already trying?
	if (eConnected == CONNECT_PENDING)
		return(FALSE);

		// Already trying?
	if (eConnected == CONNECT_SUCCESS)
		TerminateConnection();

		// Use the async call with ConnectCallback as the callback func
	if (AsyncConnectToServer(InternalCallback, NULL, lpszHostAddress, nHostPort, dwTimeout, lpszClientAddress))
		cConnectedEvent.Lock();
	else
		eConnected = CONNECT_ATTEMPT_FAILED;

			// Reset callback
	pCPLFunc = NULL;

    return(eConnected == CONNECT_SUCCESS);
};



	// An asynchronous attempt to connect to the server.
	// If submitted successfully, returns TRUE.
	// User can check eConnected for result during the callback.
BOOL CPClient::AsyncConnectToServer(CPCLIENT_CALLBACK pUserFunc, 
									void *pUserParm, 
									LPCSTR lpszHostAddress, 
									UINT nHostPort, 
									DWORD dwTimeout,
									LPCSTR lpszClientAddress)
{
	struct hostent			*lphost;
	CPClientHandler			*pClientHandler;
	int						iLeastActive;
	int						iX;
	BOOL					bPassedToHandler;

		// Can't do async at the same time
	if (!pUserFunc)
		return(FALSE);

		// Already trying?
	if (eConnected == CONNECT_PENDING)
		return(TRUE);

		// Already connected?
	if (eConnected == CONNECT_SUCCESS)
		TerminateConnection();

		// Create our socket.  ::bind or ::connect will establish the port.
	if (!Create(AF_INET, SOCK_STREAM, 0, 0, lpszClientAddress, FALSE, FALSE))
		return(FALSE);

        // Make sure we have a host address
	if(!lpszHostAddress)
		return(FALSE);

        // Set up socket structure
    memset(&sAddr, 0, sizeof(sAddr));

	sAddr.sin_family = AF_INET;
	sAddr.sin_addr.s_addr = inet_addr(lpszHostAddress);

        // Name instead of IP address?
  	if(sAddr.sin_addr.s_addr == INADDR_NONE)
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
			eConnected = CONNECT_FAILURE;
			return(FALSE);
			};
		}

        // Load port
	sAddr.sin_port = htons((u_short) nHostPort);

		// Save parms
	sLastHost = lpszHostAddress;
	iLastPort = nHostPort;

		// Actually do the connect
	eConnected = CONNECT_PENDING;
	::connect(sSocket, (struct sockaddr *) &sAddr, sizeof(sAddr));

        // Pass to a client handler
	if (iNumClientHandlers)
		{
		pClientHandler = cClientHandlers[0];
		iLeastActive = pClientHandler->GetLastSize();
		for (iX = 1; iX < iNumClientHandlers; iX++)
			{
			if (cClientHandlers[iX]->GetCurrentSize() < iLeastActive)
				pClientHandler = cClientHandlers[iX];
			};
			// Prepare to add it.  If pCPLFunc != NULL, handler will call back.
		pCPLFunc = pUserFunc;
		pUserConnectParm = pUserParm;
		ulTimeout = dwTimeout;
		ulAttemptTS = ::XPlatGetMilliseconds();
		bPassedToHandler = pClientHandler->AddConnection(this);
		}
	else
		{
		bPassedToHandler = FALSE;
		};

	if (!bPassedToHandler)
		{
		CloseSocket();
		eConnected = CONNECT_ATTEMPT_FAILED;
		return(FALSE);
		};			

	return(TRUE);
};			


	// Same as the callback below, but on an instance instead of global level
void CPClient::InstanceConnectCallback(BOOL bConnectResult)
{
	int						iLeastActive;
	int						iX;
	CPConnectionHandler		*pConnHandler;

		// Process failure
	if (!bConnectResult)
        {   // Timed out or unsucessful
        UnprotectedCloseSocket();
		if (pCPLFunc)
			pCPLFunc(this, bConnectResult, pUserConnectParm);
        return;
        };

		// Success. Pass to a connection handler
	if (iNumConnHandlers)
		{
		pConnHandler = cConnectionHandlers[0];
		iLeastActive = pConnHandler->GetLastSize();
		for (iX = 1; iX < iNumConnHandlers; iX++)
			{
			if (cConnectionHandlers[iX]->GetCurrentSize() < iLeastActive)
				pConnHandler = cConnectionHandlers[iX];
			};
		pConnHandler->AddConnection(this);	// Passing our own socket
		}
	else
		{
		bConnectResult = FALSE;	// Couldn't get a connection handler
		UnprotectedCloseSocket();
		};	

	if (pCPLFunc)
		pCPLFunc(this, bConnectResult, pUserConnectParm);

	pCPLFunc = NULL;
	pUserConnectParm = NULL;
};



	// Called from the handler
void CPClient::ConnectCallback(CPClient *pClient, BOOL bConnectResult)	
{
	pClient->InstanceConnectCallback(bConnectResult);
};


	// When removed from the handler, this gets called
void CPClient::Disconnected()
{
	eConnected = CONNECT_DISCONNECTED;
};


	// Get the last error
int	CPClient::GetConnectionError()
{
	return(iConnectError);
};


	// Get last connect attempt parameters
void CPClient::GetLastConnectAttemptParms(string &sHost, int &iPort)
{
	sHost = sLastHost;
	iPort = iLastPort;
};


	// Our own internal "user callback"
void CPClient::InternalCallback(CPClient *pClient, BOOL bConnectResult, void *)
{
	pClient->cConnectedEvent.SetEvent();
};	
