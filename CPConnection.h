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
//! CPConnection.h: interface for the CPConnection class.

#if !defined(AFX_CPCONNECTION_H__34ACB6B3_8B03_11D3_AFA6_00C04F6E1532__INCLUDED_)
#define AFX_CPCONNECTION_H__34ACB6B3_8B03_11D3_AFA6_00C04F6E1532__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "CBaseSocket.h"

class CPConnection;

//! Unique signature for each connection

class CConnectionSig
{
public:

    CConnectionSig::CConnectionSig()
    {
    };

    ~CConnectionSig()
    {
    };

    //! Operators

    friend bool operator==(const CConnectionSig& csig1, const CConnectionSig& csig2)
    {
        if (csig1.pConnection == csig2.pConnection && csig1.ulTimeStamp == csig2.ulTimeStamp)
            return (TRUE);
        else
            return (FALSE);
    };

    friend bool operator<(const CConnectionSig& csig1, const CConnectionSig& csig2)
    {
        if (csig1.pConnection < csig2.pConnection)
            return (TRUE);
        if (csig1.pConnection == csig2.pConnection && csig1.ulTimeStamp < csig2.ulTimeStamp)
            return (TRUE);
        return (FALSE);
    };

    friend bool operator>(const CConnectionSig& csig1, const CConnectionSig& csig2)
    {
        if (csig1.pConnection > csig2.pConnection)
            return (TRUE);
        if (csig1.pConnection == csig2.pConnection && csig1.ulTimeStamp > csig2.ulTimeStamp)
            return (TRUE);
        return (FALSE);
    };
public:
    CPConnection *pConnection;
    unsigned long ulTimeStamp;
};


//!	CPConnection - base class for a socket connection that can be 
//!  handled by a CPHandler socket connection handler.
class CPServer;
class CPHandler;
class CPConnectionHandler;

class CPConnection : public CBaseSocket
{
    friend CPHandler;
    friend CPConnectionHandler;
    friend CPServer;
    //! Operations
public:

    enum
    {
        CS_BUFFER_SIZE = 0x2000, //! 8K
        CS_OUTBUFFER_SIZE = 0x8000 //! 32K
    };

    CPConnection(CBaseSocket *pOwner, struct sockaddr *pAddr = NULL, int iMaxSendBufferSize = CS_OUTBUFFER_SIZE);
    virtual ~CPConnection(); //! No public destructor

    enum ECONNECT
    {
        CONNECT_UNUSED,
        CONNECT_PENDING,
        CONNECT_ATTEMPT_FAILED,
        CONNECT_SUCCESS,
        CONNECT_CLOSE_REQUESTED,
        CONNECT_DISCONNECTED,
        CONNECT_FAILURE
    };

    virtual BOOL Create(int af, //! Address family.  Only tested with AF_INET.
        int type, //! Stream or datagram (SOCK_STREAM or SOCK_DRAM)
        int protocol, //! Usually 0 for inet sockets
        UINT nPort = 0, //! Port
        LPCSTR lpAddr = NULL, //! Inet address in the standard xxx.xxx.xxx.xxx format
        BOOL bReuse = FALSE, //! Allow reuse of this socket
        BOOL bBind = TRUE, //! Default to bind the socket.  Not needed if this is the result of an ::accept
        int iLinger = -1); //! Removes from handler queue by closing socket
    //! Terminate a connection and close the socket
    virtual void TerminateConnection();
    //! Return the time the connection was initiated
    virtual time_t GetTimeConnected();
    //! Get socket address
    virtual struct sockaddr_in GetSocketAddress();
    //! Asynchronously close the socket.  If a handler hasn't
    //! been assigned, this calls TerminateConnection.
    //! Otherwise, the handler waits until the send buffer is
    //! clear and then closes the socket.
    virtual void AsyncTerminateConnection();
    //! Get connection status
    ECONNECT GetConnectionStatus();
    //! Has all data submitted by send been transferred?
    //! Only valid after AsyncTerminateConnection has been called.
    BOOL IsSendComplete();

protected:
    //! Receive the data to a socket
    virtual int ReceiveData();
    //! Send data from a socket
    virtual int SendData();
    //! Send data.  If iMaxSendBufferSize is exceeded, the return will be FALSE,
    //! and the sender must wait until some data is sent before proceeding.
    virtual BOOL SendDataOut(LPSTR lpData, int nPacketSize, int nTotalLength);
    //! Pure virtual function to process data
    virtual BOOL ProcessData(unsigned char *lpData, int iLen) = 0;
    //! Get handshake.  Return TRUE if ptr and int filled
    //! This is sent when a connection is established.
    //! Redefine to have server send a connection message.

    virtual BOOL GetHandshake(unsigned char **ucHS, int &iLen)
    {
        return (FALSE);
    };
    //! When removed from the handler, this gets called

    virtual void Disconnected()
    {
        return;
    };
    //! Unprotected close socket
    virtual void UnprotectedCloseSocket();

protected:
    //! Called by the handler itself when it accepts the connection
    virtual void SetHandler(CPHandler *pHandler);
    //! Called by SetHandler to indicate that a connection is about about to be handled
    //! SetHandler is called within CPHandler::AddConnection

    virtual int HandleConnection(void *pParm = NULL)
    {
        return 0;
    };

public:
    BOOL bRecvPause;

protected:
    CPHandler *cPHandler;
    int iMaxSendBuffer;
    //! Input buffer
    unsigned char ucBuffer[CS_BUFFER_SIZE]; 
	//! Output buffer
    unsigned char *pOutBuffer; 
	//! Current size of the output buffer
    int iOBSize; 
	//! What's left to send in the out buffer
    int iLeftToSend;
	//! Index of bytes to send 
    int iSendIndex; 
    CBaseSocket *cOwner;
    time_t tConnected;
    struct sockaddr_in sAddr;
	//! There is data to be written
    BOOL bSendData; 
	//! No room left in send buffer.  Block until some sending occurs
    BOOL bSendBufferHighWaterMark;
    ECONNECT eConnected;
    CConnectionSig cSignature;
	//! Timestamp of when a close was requested
    long lCloseRequestTS; 
    int iTotalRecvd;
    int iTotalSent;
    int iTotalSendData;
};

#endif //! !defined(AFX_CPCONNECTION_H__34ACB6B3_8B03_11D3_AFA6_00C04F6E1532__INCLUDED_)
