// CEchoConnection.cpp: implementation of the CEchoConnection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CEchoConnection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEchoConnection::CEchoConnection(	CBaseSocket *pOwner, 
									struct sockaddr *pAddr, 
									int iMaxSendBufferSize) : CPConnection(	pOwner, pAddr, iMaxSendBufferSize)
{

}

CEchoConnection::~CEchoConnection()
{

}


	// Function to process data
BOOL CEchoConnection::ProcessData(unsigned char *lpData, int iLen)
{
		// Echo the data
	SendDataOut((char *) lpData, 0, iLen);

	return(TRUE);
};
