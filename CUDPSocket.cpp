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
// CUDPSocket.cpp: implementation of the CUDPSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "CUDPSocket.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUDPSocket::CUDPSocket()
{
};

CUDPSocket::~CUDPSocket()
{
	StopThread();
};

BOOL CUDPSocket::Create(UINT nPort, LPCSTR lpAddr, BOOL bReuse, BOOL bBind)
{
	if (!CBaseSocket::Create(AF_INET, SOCK_DGRAM, 0, nPort, lpAddr, bReuse, bBind))
		return FALSE;

	return TRUE;
};


unsigned CUDPSocket::WorkerFunction(void *pVoid)
{
    unsigned rc;
    CUDPSocket *pcUDPSocket;
    
    if (!pVoid)
        return(0);
    
    pcUDPSocket = (CUDPSocket *)pVoid;

	while (TRUE)
	{
        rc = pcUDPSocket->HandleReceiveData();
        // If signaled, quit
        if (cStopEvent.Lock( 1 ) != XPLAT_TIMEOUT) 
            break;
    }

    return rc;
};

int CUDPSocket::HandleReceiveData()
{
	fd_set				readsets;
	fd_set				exceptsets;
	int					nReturn;
    int    rc;
    int    iReceived;
    int    iSize=sizeof(ucBuffer);
    struct  sockaddr_in addrFrom;
    XPLAT_ADDR_FROM_LEN 	addrLen = sizeof(addrFrom);
    
    FD_ZERO(&readsets);
	FD_ZERO(&exceptsets);
	while (TRUE)
	{
		cCriticalSocket.Lock();
		if (sSocket == INVALID_SOCKET)
		{
			cCriticalSocket.Unlock();	
			rc = -1;
			break;
		}

		FD_SET(sSocket, &readsets);
		FD_SET(sSocket, &exceptsets);
		cCriticalSocket.Unlock();

		nReturn = ::select(sSocket + 1, (XPLAT_FD_SET_PTR)&readsets, NULL, (XPLAT_FD_SET_PTR)&exceptsets, NULL);
      if (nReturn == SOCKET_ERROR)
		{
			CloseSocket();
			rc = -1;
			break;
		}
		
		cCriticalSocket.Lock();	
		if (sSocket == INVALID_SOCKET)
		{
			cCriticalSocket.Unlock();	
			rc = -1;
			break;
		}
		
		if (FD_ISSET(sSocket, &readsets))
		{
			iReceived = ::recvfrom(sSocket, (char*)ucBuffer, iSize, 0, (struct sockaddr *)&addrFrom, &addrLen);
			cCriticalSocket.Unlock();
			if (iReceived == SOCKET_ERROR)
			{
            rc = XPLAT_WSAGETLASTERR;			
         }
			else
			{
				ProcessData(ucBuffer, iReceived, &addrFrom);
			}
		}
		else
		{
			cCriticalSocket.Unlock();
		}

		cCriticalSocket.Lock();	
		if (sSocket == INVALID_SOCKET)
		{
			cCriticalSocket.Unlock();	
			rc = -1;
			break;
		}

		FD_ZERO(&readsets);
		FD_ZERO(&exceptsets);
		cCriticalSocket.Unlock();
	}

    return rc;
};

int CUDPSocket::SendMsg(unsigned char *lpData, int iLength, char *pIPAddress, int iPort)
{
	sockaddr_in		sDestAddr;

    int iSent = SOCKET_ERROR;

	cCriticalSocket.Lock();	
    if (sSocket == INVALID_SOCKET)
	{
        cCriticalSocket.Unlock();	
		return iSent;
	}

		// Set up the sockaddr structure
	sDestAddr.sin_family = AF_INET;
	sDestAddr.sin_addr.s_addr = ::inet_addr(pIPAddress);
	sDestAddr.sin_port = htons(iPort);

	iSent = ::sendto(sSocket, (char*)lpData, iLength, 0, (struct sockaddr *) &sDestAddr, sizeof(struct sockaddr_in));
	cCriticalSocket.Unlock();	

    if (iSent == SOCKET_ERROR)
    {
        int iError;
        iError = XPLAT_WSAGETLASTERR;
        CloseSocket();
    }

	return iSent;
};

void CUDPSocket::CloseSocket()
{
	CBaseSocket::CloseSocket();
};

	// Stop the thread
void CUDPSocket::StopThread()
{
	cStopEvent.SetEvent();
    CloseSocket();
	WaitForThreadCompletion();
};


	// Set the socket's broadcast mode
void CUDPSocket::SetBroadcastMode(BOOL bOn)
{
	::setsockopt(sSocket, SOL_SOCKET, SO_BROADCAST, (char *) &bOn, sizeof(bOn));
};
