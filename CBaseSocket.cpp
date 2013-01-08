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
// CBaseSocket.cpp file
// base class for listening socket class and client socket class
//
//
#if defined(_WIN32)
#include "stdafx.h"
#endif
#include "XPlat.h"
#include "CXPlatThread.h"
#include "CBaseSocket.h"

//#define DEBUGGING


///////////////////////////////////////////////////////////////////////////////

CBaseSocket::CBaseSocket()
{
    bBlocking = TRUE;
    iCreateError = 0;
    sSocket = SOCKET_ERROR;
}

CBaseSocket::~CBaseSocket()
{
    CloseSocket();
    if (!sUnixPath.empty())
        ::unlink(sUnixPath.c_str());
}


// Create our internal socket
// Note that iLinger (SO_LINGER) has nothing to do with TIME_WAIT.  It is the amount of time that after a close that the
// socket will wait for data to be flushed.  Even so, if data is waiting to be read, the socket will send a reset (FIN).
// Mostly, you don't have to worry about it.

BOOL CBaseSocket::Create(int af, int type, int protocol, UINT nPort, LPCSTR lpAddr, BOOL bReuse, BOOL bBind, int iLinger, BOOL bNoTcpDelay)
{
    int nReturn = 0;
    sockaddr_in sockAddr;
    sockaddr_un sockUnixAddr;
    unsigned long lResult;
    BOOL bOn = TRUE;
    struct linger sLinger;

    iCreateError = 0;

    // Create the socket - umask is used in case it is a Unix socket
    sSocket = ::socket(af, type, protocol);
    if (SOCKET_ERROR == sSocket)
    {
        iCreateError = XPLAT_WSAGETLASTERR;
        return FALSE;
    };

    iAF = af;

    switch(af)
    {
        case AF_INET:
        {   // Bind the socket
            memset(&sockAddr, 0, sizeof(sockAddr));
            sockAddr.sin_family = AF_INET;

            // Was an address passed in?
            if (!lpAddr) // If not, use default
                sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            else
            {
                lResult = inet_addr(lpAddr); // Convert
                if (lResult == INADDR_NONE) // Invalid address
                {
                    CloseSocket();
                    return(FALSE);
                }
                sockAddr.sin_addr.s_addr = lResult; // Load in socket address struct
            }
            sockAddr.sin_port = htons((u_short) nPort); // Load port

            // If we can reuse, set the socket option
            if (bReuse)
                ::setsockopt(sSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &bOn, sizeof(bOn));

            // Turn off Nagle algorithm to reduce latency for small data packets
            if (bNoTcpDelay)
                ::setsockopt(sSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOn, sizeof(bOn));

            // Set the LINGER option.
            // If iLinger == 0,
            // If iLinger < 0, use the system default (don't set it),
            // If iLinger > 0, set to the user defined value.
            if (iLinger == 0)
            { // DONT_LINGER
                sLinger.l_onoff = 1; // Turn it on and set it to 0
                sLinger.l_linger = 0;
                ::setsockopt(sSocket, SOL_SOCKET, SO_LINGER, (char *) &sLinger, sizeof(sLinger));
            }
            else
            {
                if (iLinger > 0)
                { // LINGER
                    sLinger.l_onoff = 1;
                    sLinger.l_linger = iLinger;
                    ::setsockopt(sSocket, SOL_SOCKET, SO_LINGER, (char *) &sLinger, sizeof(sLinger));
                };
            };

            // Bind the socket if they say TRUE or if an explicit address was passed in
            if (bBind || lpAddr)
            {
                nReturn = ::bind(sSocket, (sockaddr *) & sockAddr, sizeof(sockAddr));
                if (nReturn == SOCKET_ERROR)
                {
                    iCreateError = XPLAT_WSAGETLASTERR;
                    return(FALSE);
                };
            };
            break;
        }
        case AF_UNIX:
            // Try nicely first
            ::memset(&sockUnixAddr, 0, sizeof(sockUnixAddr));
            sockUnixAddr.sun_family = AF_UNIX;
            ::strcpy(sockUnixAddr.sun_path, lpAddr);
            sUnixPath = lpAddr;

            // Bind it
            if (bBind)
            {
                if (::unlink(lpAddr) != 0)
                    ::remove(lpAddr);
                // The umask is NOT AND with 777.  So 777 & ~111 = 666
                mode_t mOldMask = ::umask(0111);
                //  Assign the address and connect
                nReturn = ::bind(sSocket, (sockaddr *) & sockUnixAddr, sizeof(sockUnixAddr));
                ::umask(mOldMask);
                if (nReturn == SOCKET_ERROR)
                {
                    iCreateError = XPLAT_WSAGETLASTERR;
                    return(FALSE);
                };
            }

            break;
    }

    return(nReturn != SOCKET_ERROR);
}

// Return the last create error

int CBaseSocket::GetCreateError()
{
    return(iCreateError);
};



// Get socket type
int CBaseSocket::GetSocketType()
{
    return(iAF);
}
    
    
// Make this a blocking socket

BOOL CBaseSocket::SetBlockingMode(BOOL bBlock)
{
    int ioctl_opt = bBlock ? 0 : 1;

    BOOL bSuccess;


#if defined(_WIN32)
    bSuccess = ioctlsocket(sSocket, FIONBIO, (u_long FAR *) & ioctl_opt) != SOCKET_ERROR;
    if (!bSuccess)
    {
        int iErr = GetLastError();
    };
#else
    bSuccess = ioctl(sSocket, FIONBIO, (char *) & ioctl_opt) != SOCKET_ERROR;
#endif
    if (bSuccess)
        bBlocking = bBlock;
    return(bSuccess);
}


// Close the socket

void CBaseSocket::CloseSocket()
{
    if (sSocket == SOCKET_ERROR)
        return;

    // Safely
    cCriticalSocket.Lock();
    UnprotectedCloseSocket();
    cCriticalSocket.Unlock();
}


// Unprotected close socket

void CBaseSocket::UnprotectedCloseSocket()
{
    try
    { // If there is an exception, it's a race condition, which
        // in this case means someone else is closing it.  No Problem.
        // It gets accomplished.
        SOCKET sClose = sSocket;

        if (sSocket == SOCKET_ERROR)
            return;

        sSocket = SOCKET_ERROR;
        if (sClose && sClose != SOCKET_ERROR)
            ::closesocket(sClose);
    }
    catch(...)
    {
    };
};




// Return a socket address structure with info currently bound to the socket
// Only good for current call, should be copied by caller into a local variable.

struct sockaddr_in *CBaseSocket::GetSocketName()
{
    socklen_t iLen;

    cCriticalSocket.Lock();
    // Do we have a socket?
    if (sSocket == SOCKET_ERROR)
    {
        cCriticalSocket.Unlock();
        return(NULL);
    };
    cCriticalSocket.Unlock();

    iLen = sizeof(struct sockaddr_in);
    if (!::getsockname(sSocket, (struct sockaddr *) & sTAddr, &iLen))
        return(&sTAddr);

    return(NULL);
};


// Return a socket address structure with info currently bound to the socket
// Only good for current call, should be copied by caller into a local variable.

const char *CBaseSocket::GetSocketAddressAsString()
{
    struct sockaddr_in *sAddr;

    szAddress[0] = '\0';
    sAddr = GetSocketName();
#if !defined(_WIN32)
    unsigned char S_Sock_un[4];

    memcpy(S_Sock_un, &sAddr->sin_addr, sizeof(S_Sock_un));
#endif

    if (sAddr)
        sprintf(szAddress, "%u.%u.%u.%u",
#if defined(_WIN32)
        (unsigned char) sAddr->sin_addr.S_un.S_un_b.s_b1,
        (unsigned char) sAddr->sin_addr.S_un.S_un_b.s_b2,
        (unsigned char) sAddr->sin_addr.S_un.S_un_b.s_b3,
        (unsigned char) sAddr->sin_addr.S_un.S_un_b.s_b4);
#else
        S_Sock_un[0],
        S_Sock_un[1],
        S_Sock_un[2],
        S_Sock_un[3]);
#endif

    return(szAddress);
};


