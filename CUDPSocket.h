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
//! CUDPSocket.h: interface for the CUDPSocket class.
//
//

#if !defined(AFX_CUDPSOCKET_H__2AF56A89_96E3_11D3_BDA6_00C04F4CA3B5__INCLUDED_)
#define AFX_CUDPSOCKET_H__2AF56A89_96E3_11D3_BDA6_00C04F4CA3B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "XPlat.h"
#include "CBaseSocket.h"
#include "CXPlatEvent.h"
#include "CXPlatThread.h"

//! UDP socket class
class CUDPSocket : public CBaseSocket, public CXPlatThread
{
public:
    CUDPSocket();
    virtual ~CUDPSocket();
    virtual BOOL Create(UINT nPort = 0, //! Port
            LPCSTR lpAddr = NULL, //! Inet address in the standard xxx.xxx.xxx.xxx format
            BOOL bReuse = TRUE, //! Allow reuse of this socket
            BOOL bBind = TRUE); //! Default to bind the socket.
    virtual int SendMsg(unsigned char *lpData, int iLength, char *pIPAddress, int iPort);
    virtual void SetBroadcastMode(BOOL bOn = TRUE);
    virtual void CloseSocket();
    void StopThread();

    enum {
        DATA_BUFFER_SIZE = 0x800 //! 2K
    };

protected:
    virtual unsigned WorkerFunction(void *pParam);
    virtual int HandleReceiveData();
    static unsigned _stdcall ReceiveDataThread(void* pVoid); //! call back to receive data
    virtual void ProcessData(unsigned char *lpData, int iLength, struct sockaddr_in* addrFrom) = 0; //! pure virturl, process received data

    CXPlatEvent cStopEvent;
    unsigned char ucBuffer[DATA_BUFFER_SIZE];

};

#endif // !defined(AFX_CUDPSOCKET_H__2AF56A89_96E3_11D3_BDA6_00C04F4CA3B5__INCLUDED_)
