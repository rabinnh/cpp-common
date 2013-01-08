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
//! CPClientHandler.h: interface for the CPClientHandler class.

#if !defined(AFX_PCLIENTHANDLER_H__8D3D90C3_8C69_11D3_A24D_00C04F80C576__INCLUDED_)
#define AFX_PCLIENTHANDLER_H__8D3D90C3_8C69_11D3_A24D_00C04F80C576__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "XPlat.h"
#include "stldef.h"
#include "CPConnection.h"
#include "CXPlatCriticalSection.h"
#include "CXPlatThread.h"
#include "CPHandler.h"

class CPClientHandler;

typedef	vector<CPClientHandler *>	CP_CLIENT_HANDLER_ARRAY;

//! CPClienthandler is a specialized CPHandler that acts as a TCP client,
//! intiating a connection.
class CPClientHandler : public CPHandler
{
public:
	CPClientHandler();
	virtual ~CPClientHandler();

			//! Call the CPConnection->HandleConnection
			//! This is here so derived classes can choose NOT to call it
			//! in case the handler is chained (like this one)
	virtual	int			CallHandleConnection(CPConnection *pConnection) { return 0; };

protected:
			//! Function that does the work
	virtual	unsigned	WorkerFunction(void *pParam);	
			//! If we want to call the sockets Disconnected member
	virtual	void		CallDisconnected(CPConnection *pConnection) {};
};


#endif //! !defined(AFX_PCLIENTHANDLER_H__8D3D90C3_8C69_11D3_A24D_00C04F80C576__INCLUDED_)
