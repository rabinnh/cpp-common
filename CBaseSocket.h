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
//! base class for socket
//
//	Created: Richard A. Bross
//

#ifndef _CBASESOCKET_H
#define _CBASESOCKET_H

#include "XPlat.h"

#include "CXPlatCriticalSection.h"
#include <sys/un.h>


// This define really has nothing to do with kernel 2.4.  It is here because
// Linux sockets behave a bit differently from Windows and some other platforms.
// Specifically, on other platforms if you do a ::select call on a socket,
// and close the socket in another thread, the ::select will unblock.  Not on Linux.
// So you have to timeout on Linux and then check the socket itself.
#define KERNEL_2_4

//! Base socket class implements low level Berkley socket functionality

class CBaseSocket
{   //! constructor
public:
    CBaseSocket();
    virtual ~CBaseSocket();
    //! methods
public:
    virtual BOOL Create(int af, //! Address family.  Only tested with AF_INET.
            int type, //! Stream or datagram (SOCK_STREAM or SOCK_DRAM)
            int protocol, //! Usually 0 for inet sockets
            UINT nPort = 0, //! Port
            LPCSTR lpAddr = NULL, //! Address.  For inet in the standard xxx.xxx.xxx.xxx format, for AF_UNIX the path
            BOOL bReuse = FALSE, //! Allow reuse of this socket
            BOOL bBind = TRUE, //! Default to bind the socket.  Not needed if this is the result of an ::accept
            int iLinger = -1, //! -1 = system default, 0 = DONT_LINGER, else the timeout
            BOOL bNoTcpDelay = FALSE);//! FALSE = system default, TRUE = disable Nagle algorithm and send partial data
    //! Make this a blocking socket
    virtual BOOL SetBlockingMode(BOOL bBlock = TRUE);
    //! Close socket
    virtual void CloseSocket();
    //! Return a socket address structure with info currently bound to the socket
    virtual struct sockaddr_in *GetSocketName();
    //! Return the socket address as a string
    virtual const char *GetSocketAddressAsString();
    //! Return the last create error
    virtual int GetCreateError();
    //! Get socket type
    virtual int GetSocketType();

protected:
    //! Unprotected close socket - called by CloseSocket
    virtual void UnprotectedCloseSocket();


protected:
    SOCKET sSocket;
    CXPlatCriticalSection cCriticalSocket;
    BOOL bBlocking;
    struct sockaddr_in sTAddr;
    int iCreateError;
    string sUnixPath;
    int iAF;    // Address family

private:
    char szAddress[16];
};

#endif	//! !_CSOCKET_H

