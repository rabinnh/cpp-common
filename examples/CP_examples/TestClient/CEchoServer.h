// CEchoServer.h: interface for the CEchoServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CECHOSERVER_H__3F2A00A7_8BAD_11D3_8326_00A0CC20AAD9__INCLUDED_)
#define AFX_CECHOSERVER_H__3F2A00A7_8BAD_11D3_8326_00A0CC20AAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CPServer.h"

class CEchoServer : public CPServer  
{
public:
	CEchoServer(CP_HANDLER_ARRAY &cPHandlers);
	virtual ~CEchoServer();

		// Redefine in derived class
	CPConnection	*AllocateSocketClass(struct sockaddr *pAddr);
};

#endif // !defined(AFX_CECHOSERVER_H__3F2A00A7_8BAD_11D3_8326_00A0CC20AAD9__INCLUDED_)
