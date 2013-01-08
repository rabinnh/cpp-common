// TestServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "XPlat.h"
#include "stldef.h"
#include "CXPlatThread.h"
#include "CBaseSocket.h"
#include "CPConnection.h"
#include "CPConnectionHandler.h"
#include "CPServer.h"
#include "CEchoConnection.h"
#include "CEchoServer.h"
#include "conio.h"


int main(int argc, char* argv[])
{
	CEchoServer						*pServer;
	CP_CONNECTION_HANDLER_ARRAY		cHArray;
	CPConnectionHandler				cHandler;
	WSADATA							wsaData; 

	::WSAStartup(MAKEWORD(2, 0), &wsaData); 

	cHArray.push_back(&cHandler);

	pServer = new CEchoServer(cHArray);
	pServer->Create(AF_INET, SOCK_STREAM, 0, 8016, NULL);
	pServer->Listen();
	while(!kbhit())
		Sleep(1000);

	delete pServer;
	
	::WSACleanup();

	return 0;
}
