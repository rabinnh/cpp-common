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
//! CConnectionSocket header file
//!
//!	Created: Richard A. Bross
//!

#ifndef _CCONNECTIONSOCKET_H
#define _CCONNECTIONSOCKET_H

#include "CXPlatThread.h"
#include "CBaseSocket.h"

class CBaseSocket;

//! CConnectionSocket - Establish a connection with the caller

class CConnectionSocket : public CBaseSocket
{
    //! Operations
public:
    CConnectionSocket(CBaseSocket *pOwner, struct sockaddr *pAddr = NULL);
    virtual ~CConnectionSocket();

    enum {
        CS_BUFFER_SIZE = 0x2000 //! 8K
    };
    
    enum ECLOSE_TYPE {
        CLOSE_ACTIVE = 0,
        CLOSE_WAIT = 1,
        CLOSE_IMMEDIATE = 2
    };


    //! Methods

    //! Starts thread to service client.  Returns thread handle.
    virtual XPLAT_HANDLE ServiceConnection(SOCKET sNewSocket);
    //! Closes service thread by closing socket
    virtual void TerminateConnection(ECLOSE_TYPE eRequest = CLOSE_WAIT);
    //! Return the time the connection was initiated
    virtual time_t GetTimeConnected();
    //! Connected?
    virtual BOOL IsConnected();
    //! Get socket address
    virtual struct sockaddr_in GetSocketAddress();

public:
    XPLAT_HANDLE hThread;

protected:
    //! Receive the data from socket, external buffer
    virtual int ReceiveData(void* lpBuf, int nBufLen);
    //! Receive the data from socket, internal buffer
    virtual int ReceiveData();
    //! Send data
    virtual BOOL SendDataOut(LPSTR lpData, int nPacketSize, int nTotalLength);
    //! Pure virtual function to process data
    virtual BOOL ProcessData(unsigned char *lpData, int iLen) = 0;
    //! Get handshake.  Return TRUE if ptr and int filled
    //! This is sent when a connection is established.
    //! Redefine to have server send a connection message.

    virtual BOOL GetHandshake(unsigned char **ucHS, int &iLen) { return (FALSE) ; };
    //! When the ServiceConnectionThread stops, this gets called

    //! Called when the socket is disconnecting
    //! WARNING: this may occasionally get called more than once - code appropriately
    virtual void Disconnected() { return; };
    //! Thread function to listen to data, etc.
#if defined(_WIN32)
    static unsigned _stdcall ServiceConnectionThread(void* pVoid);
#else
    static unsigned ServiceConnectionThread(void* pVoid);
#endif
    //! Called by ServiceConnectionThread to actually do the work
    virtual int HandleConnection();

protected:
    unsigned char ucBuffer[CS_BUFFER_SIZE];
    CBaseSocket *cOwner;
    time_t tConnected;
    struct sockaddr_in sAddr;
    struct sockaddr_un sUAddr;
    ECLOSE_TYPE eRequestClose;
};

#endif	//! !_CCONNECTIONSOCKET_H
