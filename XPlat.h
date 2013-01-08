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
#if !defined(__XPLAT_H__)
#define __XPLAT_H__

#include <string>
#include <vector>

using namespace std;

// DO NOT CHANGE THIS!  It provides for scalability at a small cost in memory.
// If this is not set, each fd_set is limited to 64 sockets.
#if defined(_WIN32)
#if !defined(NO_FD_OVERRIDE)	//! Allow for low memory situation.  Define in the preprocessor.
	#undef FD_SETSIZE
	#define FD_SETSIZE	(32 << 10)	
#endif
#else
	// Linux defines __FD_SETSIZE in sys/types.h to 1024 and does not allow 
	// redefinition.  Therefore, you must use multiple threads handling 1024
	// max sockets apiece to achieve scalability.
#endif

#if !defined(_WIN32)
#define WAIT_OBJECT_0	0
#endif

#if defined(_WIN32)
	#include <winsock2.h>
	#include <windows.h>
	#include <process.h>
#else
		//! Includes
    #if defined(__xlC__)     //! C++Set compiler
    #include <strings.h>
    #endif
	#include <unistd.h>
	#include <pthread.h>
	#include <signal.h>
	#include <errno.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
    #if defined(__sun)
    #include <sys/filio.h>
    #endif
	#include <arpa/inet.h>
	#include <sys/param.h>
	#include <sys/resource.h>
	#include <netdb.h>
	#include <semaphore.h>
	#include <sys/ioctl.h>
		//! Defines
    #define pthread_mutexattr_default NULL
    #define pthread_condattr_default  NULL
    #define pthread_attr_default      NULL
	#define MAX_PATH PATH_MAX
	#ifdef SOCKET
	#undef SOCKET
	#endif
	typedef int SOCKET;
	#ifndef INADDR_NONE
	#define INADDR_NONE -1
	#endif
	#define closesocket(s) close(s)
	#define SOCKET_ERROR    -1
	#define GET_ERRNO errno
	#define _stdcall
//	#include "CXPlatCriticalSection.h"
#endif //! #else (_WIN32)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#if !defined(_WIN32)
typedef	void *(* START_ROUTINE)(void *);	//! For pthread_create
#endif

#if defined(_WIN32)
	#define XPLAT_HANDLE		HANDLE				
#else
	#define XPLAT_HANDLE		pthread_t
#endif


#if defined(_WIN32)
#define	PATH_CHAR	'\\'
#define sleep(s)			Sleep(s * 1000)
#define	socklen_t	int
#define XPLAT_ADDR_FROM_LEN int
#define XPLAT_FD_SET_PTR fd_set *
#define XPLAT_WSAGETLASTERR ::WSAGetLastError()
#define	XPLAT_TIMEOUT	WAIT_TIMEOUT
#endif

#if defined(_WIN32)
#define XPLAT_HMODULE	HMODULE
#else
#define XPLAT_HMODULE    void *
#define FARPROC void *
#endif

#if !defined(SOCKADDR_IN)
#define	SOCKADDR_IN		sockaddr_in
#endif

// Define Windows specifc types
#if !defined(_WIN32)

#define		XPLAT_TIMEOUT	ETIMEDOUT

#if defined(__sun)
extern "C" int ftime(struct timeb *);
#endif

#define PATH_CHAR	'/'
#define Sleep(s)			usleep(max((s * 1000), 10))

#if !defined(socklen_t) && !defined(__linux)
#define socklen_t int
#endif

#if defined(__linux)
	#define XPLAT_ADDR_FROM_LEN socklen_t
#else
	#define XPLAT_ADDR_FROM_LEN int
#endif

#define XPLAT_FD_SET_PTR fd_set *

#define XPLAT_WSAGETLASTERR errno

#define EVENTLOG_ERROR_TYPE			0 
#define	EVENTLOG_WARNING_TYPE		1
#define EVENTLOG_INFORMATION_TYPE	2
#define LPSTR char *
#define LPCSTR const char *
#define BOOL int
#define DWORD unsigned long
#define INFINITE	0xFFFFFFFF 
#define TRUE	1
#define FALSE	0
#define UINT unsigned int
#define WORD unsigned int
typedef void (*DESTRUCTOR_FUNC)(void *);
#define LONG long
#define ULONG unsigned long
#define LPCTSTR const char *
#define LPVOID void *
#define UINT unsigned int
#define BYTE unsigned char
#define PBYTE unsigned char *
#define IN_ADDR struct in_addr
#define WINAPI
#define _cdecl
#endif

// Replicate Windows specific functions
#if !defined(_WIN32)
char *_strupr(char *string1);
char *_strlwr(char *string1);
char *itoa(int value, char *szString, int radix);
#define	_itoa		itoa
#define	_itol		itoa
#define itol		_itol
#define strnicmp    strncasecmp
#define stricmp		strcasecmp
#define _strnicmp	strnicmp
#define _stricmp	stricmp
#define strlwr		_strlwr
#define strupr		_strupr
#if !defined(strcmpi)
#define strcmpi		_stricmp
#define _strcmpi	strcmpi
#endif
// NOTE: These global declarations of min and max interfere with <iostream>
//#define min(a,b) ((a) < (b) ? (a) : (b))
//#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

	//! For Unix implementations, cProtectCall protects non-threadsafe socket calls
#if defined(_WIN32)
#define		PROTECT_CALL
#define		UNPROTECT_CALL	
#else
class CXPlatCriticalSection;
extern		CXPlatCriticalSection	cProtectCall;
#define		PROTECT_CALL			cProtectCall.Lock();	
#define		UNPROTECT_CALL			cProtectCall.Unlock();	
#endif	

#if defined(_WIN32)
#define		XPLAT_NEWLINE			"\r\n"
#define		XPLAT_NEWLINE_LEN		2
#else
#define		XPLAT_NEWLINE			"\n"
#define		XPLAT_NEWLINE_LEN		1
#endif

#if defined(_WIN32)
#define XEWOULDBLOCK             WSAEWOULDBLOCK
#define XEINPROGRESS             WSAEINPROGRESS
#define XEALREADY                WSAEALREADY
#define XENOTSOCK                WSAENOTSOCK
#define XEDESTADDRREQ            WSAEDESTADDRREQ
#define XEMSGSIZE                WSAEMSGSIZE
#define XEPROTOTYPE              WSAEPROTOTYPE
#define XENOPROTOOPT             WSAENOPROTOOPT
#define XEPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define XESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define XEOPNOTSUPP              WSAEOPNOTSUPP
#define XEPFNOSUPPORT            WSAEPFNOSUPPORT
#define XEAFNOSUPPORT            WSAEAFNOSUPPORT
#define XEADDRINUSE              WSAEADDRINUSE
#define XEADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define XENETDOWN                WSAENETDOWN
#define XENETUNREACH             WSAENETUNREACH
#define XENETRESET               WSAENETRESET
#define XECONNABORTED            WSAECONNABORTED
#define XECONNRESET              WSAECONNRESET
#define XENOBUFS                 WSAENOBUFS
#define XEISCONN                 WSAEISCONN
#define XENOTCONN                WSAENOTCONN
#define XESHUTDOWN               WSAESHUTDOWN
#define XETOOMANYREFS            WSAETOOMANYREFS
#define XETIMEDOUT               WSAETIMEDOUT
#define XECONNREFUSED            WSAECONNREFUSED
#define XELOOP                   WSAELOOP
#define XENAMETOOLONG            WSAENAMETOOLONG
#define XEHOSTDOWN               WSAEHOSTDOWN
#define XEHOSTUNREACH            WSAEHOSTUNREACH
#define XENOTEMPTY               WSAENOTEMPTY
#define XEPROCLIM                WSAEPROCLIM
#define XEUSERS                  WSAEUSERS
#define XEDQUOT                  WSAEDQUOT
#define XESTALE                  WSAESTALE
#define XEREMOTE                 WSAEREMOTE
#else
#define XEWOULDBLOCK             EWOULDBLOCK
#define XEINPROGRESS             EINPROGRESS
#define XEALREADY                EALREADY
#define XENOTSOCK                ENOTSOCK
#define XEDESTADDRREQ            EDESTADDRREQ
#define XEMSGSIZE                EMSGSIZE
#define XEPROTOTYPE              EPROTOTYPE
#define XENOPROTOOPT             ENOPROTOOPT
#define XEPROTONOSUPPORT         EPROTONOSUPPORT
#define XESOCKTNOSUPPORT         ESOCKTNOSUPPORT
#define XEOPNOTSUPP              EOPNOTSUPP
#define XEPFNOSUPPORT            EPFNOSUPPORT
#define XEAFNOSUPPORT            EAFNOSUPPORT
#define XEADDRINUSE              EADDRINUSE
#define XEADDRNOTAVAIL           EADDRNOTAVAIL
#define XENETDOWN                ENETDOWN
#define XENETUNREACH             ENETUNREACH
#define XENETRESET               ENETRESET
#define XECONNABORTED            ECONNABORTED
#define XECONNRESET              ECONNRESET
#define XENOBUFS                 ENOBUFS
#define XEISCONN                 EISCONN
#define XENOTCONN                ENOTCONN
#define XESHUTDOWN               ESHUTDOWN
#define XETOOMANYREFS            ETOOMANYREFS
#define XETIMEDOUT               ETIMEDOUT
#define XECONNREFUSED            ECONNREFUSED
#define XELOOP                   ELOOP
#define XENAMETOOLONG            ENAMETOOLONG
#define XEHOSTDOWN               EHOSTDOWN
#define XEHOSTUNREACH            EHOSTUNREACH
#define XENOTEMPTY               ENOTEMPTY
#define XEPROCLIM                EPROCLIM
#define XEUSERS                  EUSERS
#define XEDQUOT                  EDQUOT
#define XESTALE                  ESTALE
#define XEREMOTE                 EREMOTE
#endif

// XPlat millisecond timer
unsigned long XPlatGetMilliseconds();
int XPlatForkHiddenProcess(const char *szProcess);

#if !defined(_WIN32)

// Given the time to wait in ms get the abstime
void GetAbstime(struct timespec &sTimespec, unsigned long lTimeToWaitMS);
#endif


#endif

