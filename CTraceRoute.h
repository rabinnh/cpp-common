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
//! CTraceRoute.h: interface for the CTraceRoute class.
//

#if !defined(AFX_CTRACEROUTE_H__52B1F4A5_BD28_11D2_AF6A_00C04F6E1532__INCLUDED_)
#define AFX_CTRACEROUTE_H__52B1F4A5_BD28_11D2_AF6A_00C04F6E1532__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif //! _MSC_VER >= 1000

#include "icmp.h"

//! Callback function prototype.
//! If iRTTms == -1, there was a timeout
#ifndef DWORD
#define DWORD   int
#endif
typedef int (*TR_CALLBACK)(const char *lpHop, int iRTTms, DWORD dwUserParm);

//! Class to implement traceroute
class CTraceRoute  
{
public:
    //! Call back function and user parm. User parm is passed to the callback.
    //! Typically it is a pointer to the calling object, since the callback must be static.
	CTraceRoute(TR_CALLBACK pCallback = NULL, DWORD dwUserParm = 0);
	virtual ~CTraceRoute();

				//! Do the traceroute
	int				TraceRoute(	const char *szIPAddress, 
								int iTimeoutMS, 
								int iMaxRetries, 
								const char *szLooseRoute = NULL,
                                bool bPingOnly = false);

				//! Register a callback function.  If the callback returns anything other than 0, stop.
	void			RegisterCallback(TR_CALLBACK pCallback, DWORD dwUserParm);

				//! Get last socket error
	int				GetLastSocketError();


protected:	
				//! Decode the ICMP iResponse
	int				DecodeTracerouteResponse(char *szBuf, int iBytes, struct sockaddr_in* sFrom, unsigned short &usSeq);
				//! Calculate the checksum
	unsigned short	CheckSum(unsigned short *szBuffer, int iSize); 

protected:
	TR_CALLBACK		pUserFunc;
	DWORD			dwUserParm;
	int				iLastError;
	struct in_addr	sLastAddr;
	unsigned short	uID;
};

#endif //! !defined(AFX_CTRACEROUTE_H__52B1F4A5_BD28_11D2_AF6A_00C04F6E1532__INCLUDED_)
