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
//! CPHandler.h: interface for the CPHandler class.

#if !defined(AFX_CPHandler_H__418CD0FA_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_)
#define AFX_CPHandler_H__418CD0FA_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "XPlat.h"
#include "stldef.h"
#include "CPConnection.h"
#include "CXPlatCriticalSection.h"
#include "CXPlatThread.h"

typedef map<CConnectionSig, CPConnection *>	CONNECTION_MAP;
typedef CONNECTION_MAP::iterator			CONNECTION_ITERATOR;

//! CPHandler handles all the connections in a pool, adding, removing, etc.
class CPHandler : public CXPlatThread
{
public:
	CPHandler();
	virtual ~CPHandler();

			//! Add a connection to handle
	virtual	BOOL		AddConnection(CPConnection *pConnections, SOCKET NewSocket = INVALID_SOCKET);
			//! Remove a connection from our purview
	virtual	BOOL		RemoveConnection(CPConnection *pConnection);
			//! Get the last size (not necesssarily current)
	virtual	int			GetLastSize();
			//! Get the current size (will wait for critical seection)
	virtual	int			GetCurrentSize();
			//! Call the CPConnection->HandleConnection
			//! This is here so derived classes can choose NOT to call it
			//! in case the handler is chained (like the CPClientHandler)
	virtual	int			CallHandleConnection(CPConnection *pConnection);

protected:
			//! Function that does the work
	virtual	unsigned	WorkerFunction(void *pParam) = 0;	
			//! If we want to call the sockets Disconnected member
	virtual	void		CallDisconnected(CPConnection *pConnection);
			//! Called in the case of an unrecoverable error
	virtual	void		DisconnectAll();
protected:
	//! To protect cLiveConnections
	CXPlatCriticalSection		cCritMap;			
	//! Active connections
	CONNECTION_MAP				cLiveConnections;	
	//! To protect cQueuedConnections
	CXPlatCriticalSection		cCritQueuedMap;		
	//! Will add to cLiveConnections at beginning of each I/O cycle
	CONNECTION_MAP				cQueuedConnections;	
	//! # in cLiveConnections
	int							iLiveSize;			
	//! # in cLiveConnections + cQueuedConnections
	int							iSize;				
	//! Faster than querying the thread
	BOOL						bThreadRunning;		
};

#endif //! !defined(AFX_CPHandler_H__418CD0FA_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_)
