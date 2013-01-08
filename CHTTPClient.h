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
// CHTTPClient.h: interface for the CHTTPClient class.
//
//
//  Created: Richard A. Bross
//
//////////////////////////////////////////////////////////////////////

#if !defined(CHTTPCLIENT_H)
#define CHTTPCLIENT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define NO_SSL
#ifndef NO_SSL
#include "e_os.h"
#include "sslc.h"
#include "rand.h"
#endif
#include "rtminet.h"
#include "CXPlatEvent.h"
#include "CClientSocket.h"

//! CHTTPClient - make a connection and fetch a URL
class CHTTPClient : public CClientSocket  
{
public:
    CHTTPClient();
    virtual ~CHTTPClient();

	//! Connect, even through a proxy
    BOOL    CheckConnect(   const char *szHost,
                            BOOL bSecure, 
                            long lPort, 
                            const char *szProxy,
                            long lProxyPort,
                            const char *szProxyUser,
                            const char *szProxyPswd,
                            long dwTimeout);

	
    BOOL    GetHTTPBytesReceived(long &dwLength);
    BOOL    GetHTTPContent(char *lpBuffer);
    BOOL    GetHTTPContentType(char *lpBuffer);
    BOOL    GetHTTPLastModified(char *lpBuffer);
    BOOL    GetHTTPNewLocation(char *lpBuffer);
    BOOL    GetHTTPServer(char *lpBuffer);
    BOOL    GetHTTPStatus(long &dwStatus);

	//! Send a request
    BOOL    SendRequest(const char *szHost,
                        const char *szHostName,
                        const char *szPath,
                        const char *szUserID,
                        const char *szPassword,
                        long  lPort,
                        const char *szProxy,
                        long  lProxyPort,
                        const char *szProxyUser,
                        const char *szProxyPswd,
                        const char *szUserAgent,
                        const char *szPost,
                        long dwTimeOut,
                        const char *szTransaction = NULL);
#ifndef NO_SSL
	//! Send an HTTPS request
    BOOL    SendSSLRequest(char *szHost,
                           char *szHostName,
                           char *szPath,
                           char *szUserID,
                           char *szPassword,
                           long  lPort,
                           char *szProxy,
                           long  lProxyPort,
                           char *szProxyUser,
                           char *szProxyPswd,
                           char *szUserAgent,
                           char *szPost,
                           long dwTimeOut,
                           SSL   **pssl = NULL,
                           SSL_CTX  **pssl_ctx = NULL,
                           char *szTransaction = NULL);
#endif

public:

    BOOL            bReadComplete;
    BOOL            bConnectFailed;

    long           dwStartTime;
    long           dwFirstByte;
    long           dwLastByte;
#ifndef NO_SSL
    SSL_SESSION     *pSession;
#endif

    string          sDebugSendHeader;
    string          sDebugRecvHeader;

protected:

    void            DeleteCookieFile(const char *lpName);
    void            Disconnected();
    long           GetTimeLeft(long dwTimeout, long dwStart);
    BOOL            ReadCookieFile(const char *lpName);
    void            ParseHeader();
    BOOL            ProcessData(unsigned char *lpData, int iLen);
    void            RequestComplete();
    void            SendCookie(char *lpBuffer);
    BOOL            WriteCookieFile(const char *lpName);

protected:

    BOOL            bHeader;
    BOOL            bHeadRequest;
    CXPlatEvent     cRecvEvent;
    string          cHeader;
    string          cHeaderLwr;
    string          cContent;
    string          cContentType;
    string          cLastModified;
    string          cLocation;
    string          cServer;
    string          cDomain;
    string          cPath;
    string          cCookieBuf;
    string          cTransaction;
    string          cRequestNum;
    long            lContentLength;
    long            lReceivedLength;
    char            szSendBuffer[INTERNET_MAX_URL_LENGTH_RTM+1];
    char            szStatus[4];
#ifndef NO_SSL
    SSL             *ssl;
    SSL_CTX         *ssl_ctx;
#endif

};

#endif
