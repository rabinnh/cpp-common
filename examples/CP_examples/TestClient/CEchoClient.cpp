// CEchoClient.cpp: implementation of the CEchoClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CEchoClient.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEchoClient::CEchoClient(CP_CONNECTION_HANDLER_ARRAY &cPCHandlers, CP_CLIENT_HANDLER_ARRAY &cPClientHandlers) :
							CPClient(cPCHandlers, cPClientHandlers)
{
}

CEchoClient::~CEchoClient()
{
}

	           // Function to process data
BOOL CEchoClient::ProcessData(unsigned char *lpData, int iLen)
{
	memset(szBuf, 0, sizeof(szBuf));
	memcpy(szBuf, lpData, min(iLen, sizeof(szBuf) - 1));
	printf(szBuf);

	return(TRUE);
}



void CEchoClient::SendString(char *pString)
{
	SendDataOut(pString, 0, strlen(pString));
};
