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
// CHTTPClient.cpp: implementation of the CHTTPClient class.
//
//
//  Created: Richard A. Bross
//
//////////////////////////////////////////////////////////////////////
#include "XPlat.h"
#if defined(_WIN32)
#include <stdafx.h>
#endif
#define NO_SSL
#ifndef NO_SSL
#include "e_os.h"
#include "sslc.h"
#include "rand.h"
#endif
#include "CXPlatThread.h"
#include "CBaseSocket.h"
#include "CConnectionSocket.h"
#include "CClientSocket.h"
#include "CHTTPClient.h"
#include "Base64Coder.h"

#if defined(_MSC_VER)
#include <direct.h>
#endif

#define     CONTENT_LENGTH    "content-length: "
#define     CONTENT_TYPE      "content-type: "
#define     LAST_MODIFIED     "last-modified: "
#define     SERVER            "server: "
#define     LOCATION          "location: "
#define     SETCOOKIE         "set-cookie: "


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHTTPClient::CHTTPClient() : CClientSocket()
{
#ifndef NO_SSL
    ssl = 0;
    ssl_ctx = 0;
#endif
}

CHTTPClient::~CHTTPClient()
{
#ifndef NO_SSL
    // Clean up ssl storage
    try
    {
        if (ssl)
        {
            SSL_free(ssl);
        }
    }
    catch(...)
    {
    }
    try
    {
        if (ssl_ctx)
        {
            SSL_CTX_free(ssl_ctx);
        }
    }
    catch(...)
    {
    }
#endif
}


// Function to process data

BOOL CHTTPClient::ProcessData(unsigned char *lpData, int iLen)
{
    char *szWork;
    char *lpStartOfContent = (char *) lpData;
    int iHeaderLen;
    int iIndex;
    char szLen[12];
    char *lpWork;

    if (!bHeader)
    {
        dwFirstByte = ::XPlatGetMilliseconds();
        szWork = strstr((char *) lpData, "\r\n\r\n");
        if (szWork)
        {
            bHeader = TRUE;
            iHeaderLen = (szWork - (char *) lpData) + 4;
            cHeader.replace(cHeader.length(), iHeaderLen, (char *) lpData, iHeaderLen);

            ParseHeader();

            if (bHeadRequest || !strcmp(szStatus, "100") || !strcmp(szStatus, "204") || !strcmp(szStatus, "304"))
            { // Only requested header or it's a no data reply so we're done
                RequestComplete();
                return(TRUE);
            };

            lpStartOfContent = szWork + 4;

            // Get content length if present
            iIndex = cHeaderLwr.find(CONTENT_LENGTH);
            if ((unsigned) iIndex != string::npos)
            {
                strncpy(szLen, &(cHeader[iIndex]) + strlen(CONTENT_LENGTH), sizeof(szLen));
                szLen[sizeof(szLen) - 1] = '\0';

                lpWork = szLen;
                while(*lpWork && !isdigit(*lpWork))
                {
                    *lpWork = ' ';
                    lpWork++;
                };
                while(*lpWork && isdigit(*lpWork))
                    lpWork++;
                if (*lpWork)
                    *lpWork = '\0';

                lContentLength = atol(szLen);
                if (!lContentLength)
                {
                    // Content length of zero, we're done
                    RequestComplete();
                };
            };

            lReceivedLength = iLen - iHeaderLen;
            if (lReceivedLength > 0)
            {
                // Save any content following the header
                cContent.replace(0, lReceivedLength, lpStartOfContent, lReceivedLength);
                if (lContentLength && (lReceivedLength >= lContentLength))
                {
                    RequestComplete();
                };
            };
            return(TRUE);
        }
        else
        {
            // Header incomplete
            cHeader += (char *) lpData;
            return(TRUE);
        };
    };

    // Have header,continue to get content
    cContent.replace(cContent.length(), iLen, lpStartOfContent, iLen);
    lReceivedLength += iLen;
    if (lContentLength && (lReceivedLength >= lContentLength))
    {
        RequestComplete();
    };
    return(TRUE);
};


// Request completed processing

void CHTTPClient::RequestComplete()
{
    dwLastByte = ::XPlatGetMilliseconds();
    bReadComplete = TRUE;

    // Header may be incomplete so try and get status anyway
    if (!bHeader)
        ParseHeader();

    cRecvEvent.SetEvent();
};


// Called when server closes socket

void CHTTPClient::Disconnected()
{
    if (!bReadComplete)
    {
        RequestComplete();
    };
};

BOOL CHTTPClient::SendRequest(const char *szHost,
    const char *szHostName,
    const char *szPath,
    const char *szUserID,
    const char *szPassword,
    long lPort,
    const char *szProxy,
    long lProxyPort,
    const char *szProxyUser,
    const char *szProxyPswd,
    const char *szUserAgent,
    const char *szPost,
    long dwTimeout,
    const char *szTransaction)

{
    char *szConnectHost;
    const char *szConnectionHeader = "Connection: close\r\n";
    const char *szContentLength = "Content-Length: %d\r\n";
    const char *szContentType = "Content-Type: application/x-www-form-urlencoded\r\n";
    const char *szProxyConn = "Proxy-Connection: Keep-Alive\r\n";
    const char *szPragma = "Pragma: no-cache\r\n";
    long lConnectPort;
    char *szBuffer = szSendBuffer;
    char szWork[256];
    const char *szGetMethod = "GET";
    const char *szPostMethod = "POST";
    char *szMethod;
    Base64Coder cBase64Coder;


    // Got to have a host
    if (!szHost)
        return(FALSE);

    // Initialize strings used for cookies
    cDomain = szHostName;
    cPath = szPath;
    if (szTransaction)
        cTransaction = szTransaction;
    else
        cTransaction = "";

    if (szPost)
        szMethod = (char *) szPostMethod;
    else
        szMethod = (char *) szGetMethod;

    if (!szProxy)
    {
        // Build request, no proxy
        szConnectHost = (char *) szHost;
        lConnectPort = lPort;

        sprintf(szBuffer, "%s %s HTTP/1.0\r\nUser-Agent: %s\r\nHost: %s\r\n", szMethod, szPath, szUserAgent, szHostName);

    }
    else
    {
        // Build request for proxy
        szConnectHost = (char *) szProxy;
        lConnectPort = lProxyPort;
        sprintf(szBuffer, "%s http://%s%s HTTP/1.0\r\nUser-Agent: %s\r\nHost: %s\r\n", szMethod, szHostName, szPath, szUserAgent, szHostName);

        if (szProxyUser && szProxyPswd && strlen(szProxyUser) > 0 && strlen(szProxyPswd) > 0)
        {
            sprintf(szWork, "%s:%s", szProxyUser, szProxyPswd);
            cBase64Coder.Encode(szWork);
            sprintf(szWork, "Proxy-Authorization: Basic %s\r\n", (LPCTSTR) cBase64Coder.EncodedMessage());
            strcat(szBuffer, szWork);
        }
    };


    if (szUserID)
    {
        sprintf(szWork, "%s:%s", szUserID, szPassword);
        cBase64Coder.Encode(szWork);
        sprintf(szWork, "Authorization: Basic %s\r\n", (LPCTSTR) cBase64Coder.EncodedMessage());
        strcat(szBuffer, szWork);
    }

    if (szPost)
    {
        strcat(szBuffer, szContentType);
        sprintf(szWork, szContentLength, strlen(szPost));
        strcat(szBuffer, szWork);
        strcat(szBuffer, szProxyConn);
        strcat(szBuffer, szPragma);
    }
    else
    {
        strcat(szBuffer, szConnectionHeader);
    }

    if (!cTransaction.empty())
        SendCookie(szBuffer);

    // Indicate end of header
    strcat(szBuffer, "\r\n");

    sDebugSendHeader = szBuffer;

    if (szPost)
        strcat(szBuffer, szPost);

    // Attempt to connect to server
    bConnectFailed = FALSE;
    if (!ConnectToServer(szConnectHost, lConnectPort, dwTimeout))
    {
        bConnectFailed = TRUE;
        return(FALSE);
    }

    // Reset variables for response
    bHeader = FALSE;
    bReadComplete = FALSE;
    bHeadRequest = FALSE;
    lReceivedLength = 0;
    lContentLength = 0;
    cHeader = "";
    cContent = "";

    dwStartTime = ::XPlatGetMilliseconds();

    // Send request
    if (!SendDataOut(szBuffer, strlen(szBuffer), strlen(szBuffer)))
    {
        TerminateConnection();
        return(FALSE);
    };

    // Wait for a reply
    if (cRecvEvent.Lock(dwTimeout) == XPLAT_TIMEOUT)
    {
        TerminateConnection();
        return(FALSE);
    };

    // Done with connection
    TerminateConnection();

    return(TRUE);
};


#ifndef NO_SSL

BOOL CHTTPClient::SendSSLRequest(char *szHost,
    char *szHostName,
    char *szPath,
    char *szUserID,
    char *szPassword,
    long lPort,
    char *szProxy,
    long lProxyPort,
    char *szProxyUser,
    char *szProxyPswd,
    char *szUserAgent,
    char *szPost,
    long dwTimeout,
    SSL **pssl,
    SSL_CTX **pssl_ctx,
    char *szTransaction)
{
    char *szConnectHost;
    char *szConnectionHeader = "Connection: close\r\n";
    char *szContentLength = "Content-Length: %d\r\n";
    char *szContentType = "Content-Type: application/x-www-form-urlencoded\r\n";
    char *szProxyConn = "Proxy-Connection: Keep-Alive\r\n";
    char *szPragma = "Pragma: no-cache\r\n";
    long lConnectPort;
    char *szBuffer = szSendBuffer;
    char szWork[256];
    char *lpWork;
    char *szGetMethod = "GET";
    char *szPostMethod = "POST";
    char *szMethod;
    BOOL RC = TRUE;
    BOOL bBIOset = FALSE;

    Base64Coder cBase64Coder;

    BIO *bio = 0;
    int done = 0, i, n;
    char buf[2048];

    // Got to have a host
    if (!szHost)
        return(FALSE);

    // Initialize strings used for cookies
    cDomain = szHostName;
    cPath = szPath;
    if (szTransaction)
        cTransaction = szTransaction;
    else
        cTransaction = "";

    // Clean up ssl storage
    try
    {
        if (ssl)
        {
            SSL_free(ssl);
            ssl = 0;
        }
    }
    catch(...)
    {
        ssl = 0;
    }

    try
    {

        // Initialise the ssl library now in rtm startup 

        // Create the context structure. Operate in the normal default SSLv3
        // in a SSLv2 header mode to maximize the number of servers
        // we can connect to.
        if (!ssl_ctx)
        {
            if (!pssl_ctx || !*pssl_ctx)
            {
                if ((ssl_ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
                {
                    bConnectFailed = TRUE;
                    throw FALSE;
                }
            }
            else
                ssl_ctx = *pssl_ctx;
        }

        // Reset variables for response
        bHeader = FALSE;
        bReadComplete = FALSE;
        bHeadRequest = FALSE;
        lReceivedLength = 0;
        lContentLength = 0;
        cHeader = "";
        cContent = "";

        // turn on all vendor bug compatibility options
        SSL_CTX_set_options(ssl_ctx, SSL_OP_ALL);

        // Create the SSL structure - defaults are inherited from the SSL_CTX
        if ((ssl = SSL_new(ssl_ctx)) == NULL)
        {
            bConnectFailed = TRUE;
            throw FALSE;
        };

        // Set as client side
        SSL_set_connect_state(ssl);

        if (!szProxy)
        {
            // No proxy
            szConnectHost = szHost;
            lConnectPort = lPort;
        }
        else
        {
            // Proxy
            szConnectHost = szProxy;
            lConnectPort = lProxyPort;
        };

        sprintf(szWork, "%s:%u", szConnectHost, (unsigned int) lConnectPort);

        // Create a BIO to handle the connect and SSL handling
        if ((bio = BIO_new_connect(szWork)) == NULL)
        {
            bConnectFailed = TRUE;
            throw FALSE;
        };

        // If using proxy then have to tunnel through to the actual ssl server
        if (szProxy)
        {
            sprintf(szBuffer, "CONNECT %s:%u HTTP/1.0\r\nUser-Agent: %s\r\n", szHostName, (unsigned int) lPort, szUserAgent);

            if (szProxyUser && szProxyPswd && strlen(szProxyUser) > 0 && strlen(szProxyPswd) > 0)
            {
                sprintf(szWork, "%s:%s", szProxyUser, szProxyPswd);
                cBase64Coder.Encode(szWork);
                sprintf(szWork, "Proxy-Authorization: Basic %s\r\n", (LPCTSTR) cBase64Coder.EncodedMessage());
                strcat(szBuffer, szWork);
            }
            strcat(szBuffer, szProxyConn);
            strcat(szBuffer, szPragma);
            strcat(szBuffer, "\r\n");

            // Send connect request
            n = strlen(szBuffer);
            i = 0;
            do
            {
                i = BIO_write(bio, &(szBuffer[i]), n);
                if (i <= 0)
                {
                    bConnectFailed = TRUE;
                    BIO_free(bio);
                    throw FALSE;
                }
                n -= i;
            }
            while(n > 0);

            // Read response from proxy
            i = BIO_read(bio, buf, sizeof(buf));
            if (i <= 0)
            {
                bConnectFailed = TRUE;
                BIO_free(bio);
                throw FALSE;
            };

            buf[i] = '\0';
            lpWork = strstr(buf, "200");
            if (!lpWork)
            {
                bConnectFailed = TRUE;
                BIO_free(bio);
                throw FALSE;
            };

        };

        // Use the newly created connect BIO
        SSL_set_bio(ssl, bio, bio);
        bBIOset = TRUE;
        if (pSession && pssl)
        {
            SSL_set_session(ssl, pSession);
        }
        else
        {
            while(!done)
            {
                int i;
                struct tm *ptr;
                time_t lt;

                lt = time(NULL);

                PROTECT_CALL
                ptr = localtime(&lt);
                char *pszTime = asctime(ptr);
                RAND_seed((unsigned char *) pszTime, strlen(pszTime));
                UNPROTECT_CALL

                // The all important handshake
                i = SSL_do_handshake(ssl);

                switch(SSL_get_error(ssl, i))
                {
                    case SSL_ERROR_NONE: // Handshake has finished, so proceed
                        done = 1;
                        break;
                    case SSL_ERROR_WANT_READ:
                    case SSL_ERROR_WANT_WRITE:
                    case SSL_ERROR_WANT_CONNECT:
                        // Perform the handshake again. 
                        sleep(1);
                        break;
                    default:
                        bConnectFailed = TRUE;
                        throw FALSE;
                        break;
                };
            };
            pSession = SSL_get_session(ssl);
        };

        if (szPost)
            szMethod = szPostMethod;
        else
            szMethod = szGetMethod;

        // Build the request
        sprintf(szBuffer, "%s %s HTTP/1.0\r\nUser-Agent: %s\r\nHost: %s\r\n", szMethod, szPath, szUserAgent, szHostName);

        if (szUserID)
        {
            sprintf(szWork, "%s:%s", szUserID, szPassword);
            cBase64Coder.Encode(szWork);
            sprintf(szWork, "Authorization: Basic %s\r\n", (LPCTSTR) cBase64Coder.EncodedMessage());
            strcat(szBuffer, szWork);
        }

        if (szPost)
        {
            strcat(szBuffer, szContentType);
            sprintf(szWork, szContentLength, strlen(szPost));
            strcat(szBuffer, szWork);
            strcat(szBuffer, szProxyConn);
            strcat(szBuffer, szPragma);
        }
        else
        {
            strcat(szBuffer, szConnectionHeader);
        }

        if (!cTransaction.empty())
            SendCookie(szBuffer);

        // Indicate end of header
        strcat(szBuffer, "\r\n");

        sDebugSendHeader = szBuffer;

        if (szPost)
            strcat(szBuffer, szPost);

        dwStartTime = ::XPlatGetMilliseconds();

        // Send request
        n = strlen(szBuffer);
        i = 0;
        do
        {
            i = SSL_write(ssl, &(szBuffer[i]), n);
            if (i <= 0)
            {
                bConnectFailed = FALSE;
                throw FALSE;
            }
            n -= i;
        }
        while(n > 0);

        // Read from the other side of the protocol
        while(1)
        {
            i = SSL_read(ssl, buf, sizeof(buf));
            if (i <= 0)
                break;

            ProcessData((unsigned char *) buf, i);

            if (!GetTimeLeft(dwTimeout, dwStartTime))
            {
                // Timed out
                bConnectFailed = FALSE;
                throw FALSE;
            };
        };
        if (szPost && !strcmp(szStatus, "100"))
        {
            printf("status 100");
            strcat(szBuffer, szPost);
            // Reset variables for response
            bHeader = FALSE;
            bReadComplete = FALSE;
            bHeadRequest = FALSE;
            lReceivedLength = 0;
            lContentLength = 0;
            cHeader = "";
            cContent = "";


            // Send request
            n = strlen(szBuffer);
            i = 0;
            do
            {
                i = SSL_write(ssl, &(szBuffer[i]), n);
                if (i <= 0)
                {
                    bConnectFailed = FALSE;
                    throw FALSE;
                }
                n -= i;
            }
            while(n > 0);

            // Read from the other side of the protocol
            while(1)
            {
                i = SSL_read(ssl, buf, sizeof(buf));
                if (i <= 0)
                    break;

                ProcessData((unsigned char *) buf, i);

                if (!GetTimeLeft(dwTimeout, dwStartTime))
                {
                    // Timed out
                    bConnectFailed = FALSE;
                    throw FALSE;
                };
            };
        };
        Disconnected();
        throw TRUE;
    }

    catch(BOOL rc)
    {
        RC = rc;

    }

    catch(...)
    {
        RC = FALSE;
        if (!bBIOset && bio)
        {
            try
            {
                BIO_free(bio);
            }
            catch(...)
            {
            }
        }
    }

    if (pssl && pssl_ctx)
    {
        if (*pssl == 0)
        {
            *pssl = ssl;
            *pssl_ctx = ssl_ctx;
            ssl = 0;
        }
        ssl_ctx = 0;
    }

    return RC;

};
#endif

void CHTTPClient::ParseHeader()
{
    string sWork;
    string sWorkLwr;
    string sValue;
    string sDomain;
    string sPath;
    string sDomainPath;
    int iIndex;
    int iEnd;
    int iStart;
    char szLine[256];


    memset(szStatus, 0, sizeof(szStatus));

    if (cHeader.length() < 12)
        return;

    sDebugRecvHeader = cHeader;

    cHeaderLwr = cHeader.c_str();
    strlwr((char *) cHeaderLwr.c_str());

    // Header starts with HTTP
    iIndex = cHeaderLwr.find("http");
    if (iIndex != 0)
        return;

    // Status code
    sWork = cHeader.substr(9, 3);
    strcpy(szStatus, sWork.c_str());

    // Content type
    iIndex = cHeaderLwr.find(CONTENT_TYPE);
    if ((unsigned) iIndex != string::npos)
    {
        iEnd = cHeader.find("\r\n", iIndex);
        if ((unsigned) iEnd != string::npos)
        {
            cContentType = cHeader.substr(iIndex + strlen(CONTENT_TYPE), iEnd - (iIndex + strlen(CONTENT_TYPE)));
        }
    }
    // Server
    iIndex = cHeaderLwr.find(SERVER);
    if ((unsigned) iIndex != string::npos)
    {
        iEnd = cHeader.find("\r\n", iIndex);
        if ((unsigned) iEnd != string::npos)
        {
            cServer = cHeader.substr(iIndex + strlen(SERVER), iEnd - (iIndex + strlen(SERVER)));
        }
    }
    // Last modified
    iIndex = cHeaderLwr.find(LAST_MODIFIED);
    if ((unsigned) iIndex != string::npos)
    {
        iEnd = cHeader.find("\r\n", iIndex);
        if ((unsigned) iEnd != string::npos)
        {
            cLastModified = cHeader.substr(iIndex + strlen(LAST_MODIFIED), iEnd - (iIndex + strlen(LAST_MODIFIED)));
        }
    }
    // Location
    iIndex = cHeaderLwr.find(LOCATION);
    if ((unsigned) iIndex != string::npos)
    {
        iEnd = cHeader.find("\r\n", iIndex);
        if ((unsigned) iEnd != string::npos)
        {
            cLocation = cHeader.substr(iIndex + strlen(LOCATION), iEnd - (iIndex + strlen(LOCATION)));
        }
    }

    // Last thing we do is check if this request is for a transaction
    // If it is then we check for and save any cookies
    if (cTransaction.empty())
        return;

    // Set-Cookie
    memset(szLine, 0, sizeof(szLine));

    iIndex = cHeaderLwr.find(SETCOOKIE);
    while((unsigned) iIndex != string::npos)
    {
        iEnd = cHeader.find("\r\n", iIndex);
        if ((unsigned) iEnd != string::npos)
        {
            sWork = cHeader.substr(iIndex + strlen(SETCOOKIE), iEnd - (iIndex + strlen(SETCOOKIE)) + 1);
            sWorkLwr = sWork.c_str();
            strlwr((char *) sWorkLwr.c_str());

            // Get attr-value pair
            iEnd = sWork.find_first_of(" ;\r");
            sValue = sWork.substr(0, iEnd);

            // look for any specified domain in set-cookie header
            iStart = sWorkLwr.find("domain=");
            if ((unsigned) iStart != string::npos)
            {
                iEnd = sWorkLwr.find_first_of(" ;\r", iStart);
                iStart += strlen("domain=");
                if ((unsigned) iEnd != string::npos)
                {
                    sDomain = sWork.substr(iStart, iEnd - iStart);
                }
            }

            // look for any specified path in set-cookie header
            iStart = sWorkLwr.find("path=");
            if ((unsigned) iStart != string::npos)
            {
                iEnd = sWorkLwr.find_first_of(" ;\r", iStart);
                iStart += strlen("path=");
                if ((unsigned) iEnd != string::npos)
                {
                    sPath = sWork.substr(iStart, iEnd - iStart);
                }
            }

            if (sDomain.empty())
            {
                sDomain = cDomain;
            }

            if (sPath.empty())
            {
                sPath = cPath;
            }

            sDomainPath = "dp=" + sDomain + sPath + ";";

            if (!ReadCookieFile(cTransaction.c_str()))
            {
                // Cookie file doesn't exist so just add
                cCookieBuf += sDomainPath + " " + sValue + ";\r\n";
            }
            else
            {
                // Look for all lines with this domain and path
                iStart = cCookieBuf.find(sDomainPath);
                BOOL bFound = FALSE;
                while((unsigned) iStart != string::npos)
                {
                    // Now look for match on attribute
                    iStart += sDomainPath.length() + 1;
                    iEnd = cCookieBuf.find('=', iStart);
                    if ((unsigned) iEnd != string::npos)
                    {
                        string sTest;
                        iEnd++;
                        sTest = cCookieBuf.substr(iStart, iEnd - iStart);
                        if (!sValue.compare(0, sTest.length(), sTest))
                        {
                            iEnd = cCookieBuf.find(';', iStart);
                            if ((unsigned) iEnd != string::npos)
                            {
                                cCookieBuf.replace(iStart, iEnd - iStart, sValue.c_str(), sValue.length());
                                bFound = TRUE;
                            }
                        }
                    }
                    iStart = cCookieBuf.find(sDomainPath, iStart);
                }
                if (!bFound)
                {
                    cCookieBuf += sDomainPath + " " + sValue + ";\r\n";
                }
            }
            WriteCookieFile(cTransaction.c_str());
        }
        iIndex = cHeaderLwr.find(SETCOOKIE, iIndex + strlen(SETCOOKIE));
    }
}

void CHTTPClient::SendCookie(char *lpBuffer)
{
    int iStart, iEnd, iIndex;
    string sWork;
    string sDomainPath;
    string sAttrValue;
    BOOL bFirst = TRUE;
    BOOL bFound = FALSE;

    // Get request number
    iIndex = cTransaction.find('@');
    if ((unsigned int) iIndex == string::npos)
        return;

    cRequestNum = cTransaction.substr(iIndex + 1, cTransaction.length() - iIndex + 1);
    cTransaction = cTransaction.substr(0, iIndex);

    // If this is the first request for a transaction, then we remove any previous cookies
    if (cRequestNum.compare("1") == 0)
    {
        DeleteCookieFile(cTransaction.c_str());
        return;
    }

    ReadCookieFile(cTransaction.c_str());
    if (cCookieBuf.empty())
        return;

    sDomainPath = cDomain + cPath;

    // Look for the specified domain path
    iStart = cCookieBuf.find("dp=");
    while((unsigned) iStart != string::npos)
    {
        iEnd = cCookieBuf.find_first_of(";", iStart);
        iStart += strlen("dp=");
        if ((unsigned) iEnd != string::npos)
        {
            sWork = cCookieBuf.substr(iStart, iEnd - iStart);
            iIndex = sDomainPath.find(sWork);
            if ((unsigned) iIndex != string::npos)
            {
                iStart = iEnd;
                iStart += 2;
                iEnd = cCookieBuf.find_first_of(";", iStart);
                if ((unsigned) iEnd != string::npos)
                {
                    bFound = TRUE;
                    sAttrValue = cCookieBuf.substr(iStart, iEnd - iStart);
                    if (bFirst)
                    {
                        bFirst = FALSE;
                        strcat(lpBuffer, "Cookie: ");
                        strcat(lpBuffer, sAttrValue.c_str());
                    }
                    else
                    {
                        strcat(lpBuffer, "; ");
                        strcat(lpBuffer, sAttrValue.c_str());
                    }
                }
            }
        }
        iStart = cCookieBuf.find("dp=", iStart);
    }
    if (bFound)
    {
        strcat(lpBuffer, "\r\n");
    }
}

BOOL CHTTPClient::ReadCookieFile(const char *lpName)
{
    int iRet, iLen = 0;
    char szPath[256];
    char *pTempBuf;
    FILE *pFile;
#if defined(_WIN32)
    const char *lpPath = "%s\\bin\\SilverSpore-Agent\\Cache\\%s";
#else
    const char *lpPath = "%s/bin/SilverSpore-Agent/Cache/%s";
#endif

    sprintf(szPath, lpPath, getenv("AGENTWORKS_DIR"), lpName);

    cCookieBuf = "";
    pFile = ::fopen((const char *) szPath, "rb");
    if (!pFile)
        return FALSE;

    // Get it's size
    iRet = ::fseek(pFile, 0, SEEK_END); // Returns 0 on success
    if (!iRet)
        iLen = ::ftell(pFile); // Get the length
    if (!iLen)
    {
        fclose(pFile);
        return FALSE;
    }

    // Reset to the beginning
    fseek(pFile, 0, SEEK_SET);

    // Allocate a buffer
    pTempBuf = new char[iLen + 1];
    assert(pTempBuf != NULL);

    // Read into the buffer
    iRet = fread(pTempBuf, 1, iLen, pFile);
    fclose(pFile);

    pTempBuf[iLen] = '\0';

    cCookieBuf = pTempBuf;

    if (pTempBuf)
        delete[] pTempBuf;

    return TRUE;
}

BOOL CHTTPClient::WriteCookieFile(const char *lpName)
{
    int iRet;
    char szPath[256];
    FILE *pFile;
#if defined(_WIN32)
    LPSTR lpPath = "%s\\agents\\bin\\SilverSpore-Agent\\Cache";
    // Create path in case it doesn't exist
    sprintf(szPath, lpPath, getenv("AGENTWORKS_DIR"));
    mkdir(szPath);
    strcat(szPath, "\\");
    strcat(szPath, lpName);
#else
    const char *lpPath = "%s/agents/bin/SilverSpore-Agent/Cache";
    // Create path in case it doesn't exist
    sprintf(szPath, lpPath, getenv("AGENTWORKS_DIR"));
    mkdir(szPath, 0774);
    strcat(szPath, "/");
    strcat(szPath, lpName);
#endif

    pFile = ::fopen((const char *) szPath, "wb");
    if (!pFile)
        return FALSE;

    // Write from the buffer
    iRet = fwrite(cCookieBuf.c_str(), 1, cCookieBuf.length(), pFile);
    fclose(pFile);

    return TRUE;
}

void CHTTPClient::DeleteCookieFile(const char *lpName)
{
    char szPath[256];
#if defined(_WIN32)
    LPSTR lpPath = "%s\\agents\\bin\\SilverSpore-Agent\\Cache\\%s";
#else
    const char *lpPath = "%s/agents/bin/SilverSpore-Agent/Cache/%s";
#endif

    sprintf(szPath, lpPath, getenv("AGENTWORKS_DIR"), lpName);

    ::remove((const char *) szPath);

    return;
}

BOOL CHTTPClient::GetHTTPStatus(long &dwStatus)
{
    if (!szStatus)
        return FALSE;

    dwStatus = atol(szStatus);
    if (dwStatus == 0 && !cHeader.empty())
    {
        // Did not get a header or got incomplete header.
        cContent = cHeader.c_str();
        lReceivedLength = cHeader.length();
        dwStatus = 200;
        return TRUE;
    }

    return TRUE;
};

BOOL CHTTPClient::GetHTTPBytesReceived(long &dwLength)
{
    if (!lReceivedLength)
        return FALSE;

    dwLength = lReceivedLength;
    return TRUE;
};

BOOL CHTTPClient::GetHTTPContent(char *lpBuffer)
{
    if (cContent.empty())
        return FALSE;

    memcpy(lpBuffer, cContent.c_str(), lReceivedLength);
    return TRUE;
};

BOOL CHTTPClient::GetHTTPContentType(char *lpBuffer)
{
    if (cContentType.empty())
        return FALSE;

    strcpy(lpBuffer, cContentType.c_str());
    return TRUE;
};

BOOL CHTTPClient::GetHTTPLastModified(char *lpBuffer)
{
    if (cLastModified.empty())
        return FALSE;

    strcpy(lpBuffer, cLastModified.c_str());
    return TRUE;
};

BOOL CHTTPClient::GetHTTPNewLocation(char *lpBuffer)
{
    if (cLocation.empty())
        return FALSE;

    strcpy(lpBuffer, cLocation.c_str());
    return TRUE;
};

BOOL CHTTPClient::GetHTTPServer(char *lpBuffer)
{
    if (cServer.empty())
        return FALSE;

    strcpy(lpBuffer, cServer.c_str());
    return TRUE;
};


// Get time left to complete a request

long CHTTPClient::GetTimeLeft(long dwTimeout, long dwStart)
{
    long dwTimework;

    // How much time left?
    dwTimework = ::XPlatGetMilliseconds() - dwStart;
    if ((long) dwTimework < 0) // Turnover
        dwTimework += (long) - 1;
    dwTimeout -= dwTimework;
    return((long) dwTimeout > 0 ? dwTimeout : 0);
};


// See if we can connect

BOOL CHTTPClient::CheckConnect(const char *szHost,
    BOOL bSecure,
    long lPort,
    const char *szProxy,
    long lProxyPort,
    const char *szProxyUser,
    const char *szProxyPswd,
    long dwTimeout)
{
    char *szConnectHost;
    long lConnectPort;
    char *szBuffer = (char *) szSendBuffer;

    // Got to have a host
    if (!szHost)
        return(FALSE);

    // Set variables
    if (!szProxy)
    {
        szConnectHost = (char *) szHost;
        lConnectPort = lPort;
    }
    else
    {
        sprintf(szBuffer, "HEAD %s://%s:%u/ HTTP/1.0\r\n\r\n",
            bSecure ? "https" : "http", szHost, (unsigned int) lPort);
        szConnectHost = (char *) szProxy;
        lConnectPort = lProxyPort;
    };

    // Attempt to connect
    if (!ConnectToServer(szConnectHost, lConnectPort, dwTimeout))
        return(FALSE);

    // If not using a proxy, it's alive!
    if (!szProxy)
    {
        TerminateConnection();
        return(TRUE);
    };

    // Reset variables for response
    bHeader = FALSE;
    bReadComplete = FALSE;
    bHeadRequest = TRUE;
    lReceivedLength = 0;
    lContentLength = 0;
    cHeader = "";
    cContent = "";

    // For a proxy connection, attempt a head request
    if (!SendDataOut(szBuffer, strlen(szBuffer), strlen(szBuffer)))
    {
        TerminateConnection();
        return(FALSE);
    };

    // Wait for a reply

    if (cRecvEvent.Lock(dwTimeout) == XPLAT_TIMEOUT)
    {
        TerminateConnection();
        return(FALSE);
    };

    // Only need one packet
    TerminateConnection();

    // Check the return
    if (cHeader.length() < 12)
        return(FALSE);

    // Check the code. Any that start with "5" is a proxy error
    if (cHeader[9] == '5')
        return(FALSE);

    return(TRUE);
};
