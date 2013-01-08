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
//! CPConnectionHandler.h: interface for the CPConnectionHandler class.

#if !defined(AFX_CPCONNECTIONHANDLER_H__418CD0FA_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_)
#define AFX_CPCONNECTIONHANDLER_H__418CD0FA_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "XPlat.h"
#include "stldef.h"
#include "CPConnection.h"
#include "CXPlatCriticalSection.h"
#include "CXPlatThread.h"
#include "CPHandler.h"

class CPConnectionHandler;

typedef	vector<CPConnectionHandler *>	CP_CONNECTION_HANDLER_ARRAY;

//! CPConnectionHandler does the actual I/O for each connection
class CPConnectionHandler : public CPHandler
{
public:	//! CloseWaitMilliseconds is the number of milliseconds after the last send to close the socket
	CPConnectionHandler(long lCloseWaitMilliseconds = 0);
	virtual ~CPConnectionHandler();

protected:
			//! Function that does the work
	virtual	unsigned	WorkerFunction(void *pParam);	
			//! Get MS difference between 2 times
			long		GetTimeDiff(long lStart, long lFinish);

protected:
			long		lCloseWaitMS;	//! MS to wait before closing a socket after a send
};

#endif //! !defined(AFX_CPCONNECTIONHANDLER_H__418CD0FA_8B0F_11D3_AFA6_00C04F6E1532__INCLUDED_)
