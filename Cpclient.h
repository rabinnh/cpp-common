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
//! CPClient.h: interface for the CPClient class.

#if !defined(AFX_CPCLIENT_H__48FA5EF3_8BE9_11D3_8326_00A0CC20AAD9__INCLUDED_)
#define AFX_CPCLIENT_H__48FA5EF3_8BE9_11D3_8326_00A0CC20AAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "XPlat.h"
#include "CXPlatEvent.h"
#include "CBaseSocket.h"
#include "CPConnection.h"
#include "CPHandler.h"
#include "CPConnectionHandler.h"
#include "CPClientHandler.h"

class CPClient;

//! Calls back with the CPClient, result of the attempt, and a user parameter
typedef		void (_stdcall *CPCLIENT_CALLBACK)(CPClient *, BOOL, void *);

//! Initiate a TCP client connection
class CPClient : public CPConnection  
{
friend CPClientHandler;
public:
	CPClient(	CP_CONNECTION_HANDLER_ARRAY &cPCHandlers, 
				CP_CLIENT_HANDLER_ARRAY &cPClientHandlers,
				int iMaxSendBufferSize = 16384);
	virtual ~CPClient();
	
			//! Attempt to connect to the server
virtual BOOL		ConnectToServer(LPCSTR lpszHostAddress, 
									UINT nHostPort, 
									DWORD dwTimeout = INFINITE,
									LPCSTR lpszClientAddress = NULL);
			//! An asynchronous attempt to connect to the server
virtual BOOL		AsyncConnectToServer(	CPCLIENT_CALLBACK pUserFunc, 
											void *pUserParm, 
											LPCSTR lpszHostAddress, 
											UINT nHostPort, 
											DWORD dwTimeout = INFINITE,
											LPCSTR lpszClientAddress = NULL);
			//! If you derive from this, be sure to call the CPClient::Disconnected in your derived method!
		void		Disconnected();
			//! Get the last error
		int			GetConnectionError();

protected:
			//! Same as the callback below, but on an instance instead of global level
		void		InstanceConnectCallback(BOOL bConnectResult);
			//! Get last connect attempt parameters
		void		GetLastConnectAttemptParms(string &sHost, int &iPort);
		//! Callback
#if defined(_WIN32)
static	void _stdcall ConnectCallback(CPClient *pClient, BOOL bConnectResult);	
static	void _stdcall InternalCallback(CPClient *pClient, BOOL bConnectResult, void *);	
#else
static	void ConnectCallback(CPClient *pClient, BOOL bConnectResult);	
static	void InternalCallback(CPClient *pClient, BOOL bConnectResult, void *);	
#endif

public:
	void					*pUserParm;
	long					lConnectResponse;

protected:
		//! Array of CPClientHandlers using for making connections.
	CP_CLIENT_HANDLER_ARRAY		cClientHandlers;	
	int							iNumClientHandlers;
		//! Array of CPConnectionHandlers.  Used for servicing successful connetions.
	CP_CONNECTION_HANDLER_ARRAY	cConnectionHandlers;
	int							iNumConnHandlers;
	//! Set when success or failure
	CXPlatEvent					cConnectedEvent;	
	//! Timestamp when the attempt started
	unsigned long				ulAttemptTS;		
	//! User callback function
	CPCLIENT_CALLBACK			pCPLFunc;			
	void						*pUserConnectParm;
	unsigned long				ulTimeout;
	int							iConnectError;
	string						sLastHost;
	int							iLastPort;
	long						lStartTime;
	string						sServer;
};

#endif //! !defined(AFX_CPCLIENT_H__48FA5EF3_8BE9_11D3_8326_00A0CC20AAD9__INCLUDED_)
