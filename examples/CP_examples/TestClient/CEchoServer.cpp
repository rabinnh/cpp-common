// CEchoServer.cpp: implementation of the CEchoServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CEchoConnection.h"
#include "CEchoServer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEchoServer::CEchoServer(CP_HANDLER_ARRAY &cPHandlers) : CPServer(cPHandlers)
{

}

CEchoServer::~CEchoServer()
{

}


	// Redefine in derived class
CPConnection *CEchoServer::AllocateSocketClass(struct sockaddr *pAddr)
{
	CEchoConnection		*pConnection;

	pConnection = new CEchoConnection(this, pAddr);
	assert(pConnection != NULL);
	
	return(pConnection);
};			
	
