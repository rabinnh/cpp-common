// CEchoConnection.h: interface for the CEchoConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CECHOCONNECTION_H__3F2A00A6_8BAD_11D3_8326_00A0CC20AAD9__INCLUDED_)
#define AFX_CECHOCONNECTION_H__3F2A00A6_8BAD_11D3_8326_00A0CC20AAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CPConnection.h"

class CEchoConnection : public CPConnection  
{
public:
	CEchoConnection(CBaseSocket *pOwner, struct sockaddr *pAddr = NULL, int iMaxSendBufferSize = 4096);
	virtual ~CEchoConnection();

            // Function to process data
	BOOL        ProcessData(unsigned char *lpData, int iLen);

};

#endif // !defined(AFX_CECHOCONNECTION_H__3F2A00A6_8BAD_11D3_8326_00A0CC20AAD9__INCLUDED_)
