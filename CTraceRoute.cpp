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
/// 3/1/97 Richard Bross

// CTraceRoute.cpp: implementation of the CTraceRoute class.
//
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
    #include "stdafx.h"
    #include <winsock2.h>
    #ifndef IP_TOS
        #include <ws2tcpip.h>
    #else
        #include <winsock.h>
    #endif
#endif
#include <errno.h>
#include <netinet/in.h>
#include "CTraceRoute.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTraceRoute::CTraceRoute(TR_CALLBACK pCallback, DWORD dwUserParm)
{
	RegisterCallback(pCallback, dwUserParm);
}

CTraceRoute::~CTraceRoute()
{

}

	// Register a callback function.  If the callback returns anything other than 0, stop.
void CTraceRoute::RegisterCallback(TR_CALLBACK pCallback, DWORD dwUserParm)
{
	pUserFunc = pCallback;
	this->dwUserParm = dwUserParm;
};


	// Do the traceroute
int	CTraceRoute::TraceRoute(	const char *szIPAddress,	// IPaddress to trace to
								int iTimeoutMS,				// Timeout for each ICMP packet
								int iMaxRetries,			// Max retries
								const char *szLooseRoute,   // Pass through this router
                                bool bPingOnly)	
{
	struct	sockaddr_in dest = {0};
	struct	sockaddr_in from;
	struct	timeval 	timeout;
	fd_set				socket_list = {0};
	int 				retries	= iMaxRetries;
	int 				fromlen 	= sizeof(from);
	DWORD				ioctl_param = 1;			// Enable Non-Blocking I/O.  We'll block on the select
	unsigned int		sum 		= 0;
	unsigned short		seq_no;
	int 				rc			= PING_RESPONSE_SUCCESS;
	BOOL				done;
	int 				n;
	int 				bread;
	int					ttl;
	SOCKET				sSocket;
	char				send_buf[sizeof(IcmpHeader) + DATA_PORTION_SIZE] = {0};
	char				recv_buf[IP_PACKET_MIN + sizeof(IpHeader) + DATA_PORTION_SIZE] = {0};
	IPLooseRoute		sLooseRoute;
	IcmpHeader			*icmphdr;

		// Set socket attributes: Protocol: ICMP,  Non-Blocking Mode (no_wait)
	sSocket = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sSocket == INVALID_SOCKET)
		{
		iLastError = ::WSAGetLastError();
		return(PING_RESPONSE_ERROR);
		};

	if (::ioctlsocket(sSocket, FIONBIO, &ioctl_param ) == SOCKET_ERROR)
		{
		iLastError = ::WSAGetLastError();
		return(PING_RESPONSE_ERROR);
		};

		// Do the traceroute
    ttl = 0;
    if (bPingOnly)
	    ttl = 99;
	timeout.tv_sec	= iTimeoutMS / 1000;	
	timeout.tv_usec = (iTimeoutMS - (timeout.tv_sec * 1000)) * 100000;	// Microseconds
			
	dest.sin_addr.s_addr	= inet_addr(szIPAddress);
	dest.sin_family 		= AF_INET;
	sLastAddr.S_un.S_addr	= -1;
	do
		{
		retries = iMaxRetries;
		rc = PING_RESPONSE_ERROR;
		ttl++;
		while (retries-- && (rc != PING_RESPONSE_SUCCESS && rc != PING_RESPONSE_TRACING))
			{
			FD_ZERO(&socket_list);
			FD_SET(sSocket, &socket_list);	// Store sSocket_handle in the list
	
				// Loose routing?
			if (szLooseRoute)
				{
				memset(&sLooseRoute, 0, sizeof(sLooseRoute));
				sLooseRoute.nopt = 1;	// An option is present
				sLooseRoute.code = 0x83;
				sLooseRoute.len = 11;
				sLooseRoute.ptr = 4;
				sLooseRoute.ip_addr[0] = inet_addr(szLooseRoute);
				sLooseRoute.ip_addr[1] = inet_addr(szIPAddress);
				iLastError = ::setsockopt(	sSocket,
											IPPROTO_IP,
											IP_OPTIONS,
											(char *) &sLooseRoute,
											12);
				if (iLastError == SOCKET_ERROR)
					{
					iLastError = ::WSAGetLastError();
					return(PING_RESPONSE_ERROR);
					};
				};

			iLastError = ::setsockopt(sSocket, IPPROTO_IP, IP_TTL, (char *) &ttl, sizeof(ttl));
			if (iLastError == SOCKET_ERROR)
				{
				iLastError = ::WSAGetLastError();
				return(PING_RESPONSE_ERROR);
				};

			memset(send_buf, 0, sizeof(IcmpHeader) + DATA_PORTION_SIZE);
			icmphdr = (IcmpHeader *) send_buf;
			icmphdr->i_type = ICMP_ECHO;
			icmphdr->icmp_hun.ih_idseq.icd_id =	uID = // Make unique within this app as well by using "GetTickCount"
								(unsigned short) (::GetCurrentProcessId() & ::GetTickCount() & 0xFFFF);
			icmphdr->icmp_hun.ih_idseq.icd_seq = seq_no =
								(unsigned short) (::GetTickCount() & 0xFFFF);
			icmphdr->i_cksum = CheckSum((unsigned short*) send_buf, sizeof(send_buf));

			iLastError = ::sendto(	sSocket,
									send_buf,
									sizeof(send_buf),
									0,
									(struct sockaddr *) &dest, sizeof(dest));

			if (iLastError == SOCKET_ERROR)
				{
				iLastError = ::WSAGetLastError();
				return(PING_RESPONSE_ERROR);
				};
			
				// Wait for timeout
			done = FALSE;
			rc = PING_RESPONSE_SUCCESS;
			while (!done && rc == PING_RESPONSE_SUCCESS)
				{
				n = ::select(0, &socket_list, 0, 0, &timeout);

				if (n < 0)
					rc = ::WSAGetLastError();

				if (n < 0 && errno == EINTR)
					continue;
				if ( n <= 0 )
					rc = (n == 0) ? PING_RESPONSE_TIMEOUT : PING_RESPONSE_ERROR;
				else
					{
					bread = ::recvfrom(	sSocket,
										recv_buf,
										sizeof(recv_buf),
										0, (struct sockaddr*)&from,
										&fromlen);
					done = TRUE;
					}
				};
	
					// Because of the Non_Blocking sSocket, if the 'recfrom' buffer has nothing
					// in it, then basically we timed out (i.e., the timing loop exited without
					// ever receiving anything).
			if (rc == PING_RESPONSE_SUCCESS)
				{
				if (recv_buf[0] == '\0')
					rc = PING_RESPONSE_TIMEOUT;
				else
					if (bread == SOCKET_ERROR)
						{	
						iLastError = ::WSAGetLastError();
						::closesocket(sSocket);
						return(PING_RESPONSE_ERROR);
						}
				};

			if (rc == PING_RESPONSE_TIMEOUT)
				{
				rc = 0;
				if (pUserFunc)
					rc = pUserFunc(NULL, -1, dwUserParm);
					// If return != 0, user cancelled
				if (rc)
					{
					::closesocket(sSocket);
					return(PING_RESPONSE_CANCELLED);
					};
				};

			if (rc == PING_RESPONSE_SUCCESS)
				{
				rc = DecodeTracerouteResponse(	recv_buf,
												bread,
												&from,
												seq_no);	

					// We tried source routing and it failed
				if (rc == PING_RESPONSE_SOURCEROUTEFAIL)
					{
					::closesocket(sSocket);
					return(PING_RESPONSE_SOURCEROUTEFAIL);
					};

					// Time exceeded?
				if (rc == PING_RESPONSE_TIMEOUT)
					{
					rc = 0;
					if (pUserFunc)
						rc = pUserFunc(NULL, -1, dwUserParm);
						// If return != 0, user cancelled
					if (rc)
						{
						::closesocket(sSocket);
						return(PING_RESPONSE_CANCELLED);
						};
					}
				else
					{	// If cancelled or unreachable, give it up
					if (rc == PING_RESPONSE_CANCELLED || rc == PING_RESPONSE_UNREACHABLE)
						{
						::closesocket(sSocket);
						return(rc);
						};

						// Not our packet
					if (rc != PING_RESPONSE_SUCCESS && rc != PING_RESPONSE_TRACING)	// Not our packet
						{
						retries++;	// reset retries
						rc = PING_RESPONSE_TIMEOUT;	// not really a timeout, but treat it as one
						};
					};
				};
/*	This code was a test to see if you could skip a timed out
	node and still proceed.  You can.

			if (!retries && !rc)
				{
				rc = PING_RESPONSE_TRACING;
				retries = 3;
				ttl++;
				};
*/
			} // while
		}	// do
	while(rc == PING_RESPONSE_TRACING);

	::closesocket(sSocket);

	return rc;
};




	// Get last socket error
int	CTraceRoute::GetLastSocketError()
{
	return(iLastError);
};



	// Decode the ICMP iResponse
int	CTraceRoute::DecodeTracerouteResponse(	char *szBuf,
											int	iBytes,
											struct sockaddr_in* sFrom,
											unsigned short &usSeq)
{
	IpHeader			*pIPHeader;
	IcmpHeader			*pICMPHeader;
	char				*szTempName;
	unsigned char		*pszBuf = (unsigned char *) szBuf;
	int					iReturn;
	int					iHlen;
	unsigned char		iType;
	unsigned char		iCode;
	int					iRTT;
	DWORD				dwTickCount = GetTickCount();
	
	pIPHeader = (IpHeader *) szBuf;
	iHlen = (pIPHeader->h_len << 2);
	if (iBytes <  iHlen + ICMP_PACKET_MIN)
		return ICMP_RESPONSE_TOOFEW;

		// Get position of icmp header
	iBytes -= iHlen;
	pICMPHeader = (IcmpHeader *) (pszBuf + iHlen);

	iType = pICMPHeader->i_type;
	iCode = pICMPHeader->i_code;

		// If we tried source routing, it may not be supported

	if (iType == ICMP_UNREACH && iCode == ICMP_UNREACH_SRCFAIL)
		return(PING_RESPONSE_SOURCEROUTEFAIL);

	if ((iType == ICMP_TIMXCEED && iCode == ICMP_TIMXCEED_INTRANS) ||
					iType == ICMP_UNREACH || iType == ICMP_ECHOREPLY)
		{
		pIPHeader = &pICMPHeader->icmp_dun.id_ip.idi_ip;
		iHlen = pIPHeader->h_len << 2;

		if (iType == ICMP_ECHOREPLY &&
			pICMPHeader->icmp_hun.ih_idseq.icd_id == uID)
			{
			if (sFrom->sin_addr.S_un.S_addr != sLastAddr.S_un.S_addr)
				{
				iRTT = (unsigned short) (dwTickCount & 0xFFFF) -
								pICMPHeader->icmp_hun.ih_idseq.icd_seq;
				if (iRTT < 0)
					iRTT += 0xFFFF;
				sLastAddr = sFrom->sin_addr;
				szTempName = inet_ntoa(sFrom->sin_addr);
				if (pUserFunc)
					pUserFunc(szTempName, iRTT, dwUserParm);	// Finished anyway, so no need to check for cancelled
				};
			return(PING_RESPONSE_SUCCESS);
			};

		pICMPHeader = (IcmpHeader *)((unsigned char *) pIPHeader + iHlen);
		if (pICMPHeader->icmp_hun.ih_idseq.icd_id != uID)
			return ICMP_RESPONSE_INVID;

		if (iHlen + ICMP_RETURNED_DATA <= (unsigned short) (iBytes - sizeof(IpHeader)) &&
				pIPHeader->proto == IPPROTO_ICMP &&
					pICMPHeader->icmp_hun.ih_idseq.icd_id == uID &&
						pICMPHeader->icmp_hun.ih_idseq.icd_seq == usSeq)
			return(iType == ICMP_TIMXCEED ? PING_RESPONSE_TIMEOUT : PING_RESPONSE_UNREACHABLE);
		};

	if (sFrom->sin_addr.S_un.S_addr != sLastAddr.S_un.S_addr)
		{
		iRTT = (unsigned short) (dwTickCount & 0xFFFF) -
								pICMPHeader->icmp_hun.ih_idseq.icd_seq;
		if (iRTT < 0)
			iRTT += 0xFFFF;
		sLastAddr = sFrom->sin_addr;
		szTempName = inet_ntoa(sFrom->sin_addr);
		if (pUserFunc(szTempName, iRTT, dwUserParm))
			return(PING_RESPONSE_CANCELLED);
		return(PING_RESPONSE_TRACING);
		};

		// Unreachable source fail?
	if (iType == ICMP_UNREACH && (iCode == ICMP_UNREACH_SRCFAIL || iCode > ICMP_UNREACH_TOSHOST))
		return(PING_RESPONSE_UNREACHABLE);

	iReturn = pICMPHeader->i_type == ICMP_ECHOREPLY ? PING_RESPONSE_SUCCESS : PING_RESPONSE_TRACING;
	return(iReturn);
}



	// Calculate the checksum
unsigned short CTraceRoute::CheckSum(unsigned short *usBuffer, int iSize)
{
	unsigned long lChecksum = 0;

	while(iSize > 1)
		{
		lChecksum += *usBuffer++;
		iSize -= sizeof(unsigned short);
		}

	if (iSize)
	   lChecksum += *(unsigned char *) usBuffer;

	lChecksum = (lChecksum >> 16) + (lChecksum & 0xffff);
	lChecksum += (lChecksum >> 16);

	return (unsigned short)(~lChecksum);
}

