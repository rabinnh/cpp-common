// CEchoClient.h: interface for the CEchoClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CECHOCLIENT_H__8D3D90C7_8C69_11D3_A24D_00C04F80C576__INCLUDED_)
#define AFX_CECHOCLIENT_H__8D3D90C7_8C69_11D3_A24D_00C04F80C576__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CPClient.h"

class CEchoClient : public CPClient  
{
public:
	CEchoClient(CP_CONNECTION_HANDLER_ARRAY &cPCHandlers, CP_CLIENT_HANDLER_ARRAY &cPClientHandlers);
	virtual ~CEchoClient();


		void		SendString(char *pString);
protected:
	           // Function to process data
		BOOL        ProcessData(unsigned char *lpData, int iLen);
protected:
		char		szBuf[4096];
};

#endif // !defined(AFX_CECHOCLIENT_H__8D3D90C7_8C69_11D3_A24D_00C04F80C576__INCLUDED_)
