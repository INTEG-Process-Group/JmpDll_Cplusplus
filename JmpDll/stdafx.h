// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#define no_init_all


// TODO: reference additional headers your program requires here
#ifdef _WIN32

#include <windows.h>
#include <winsock2.h>

#else


#endif




#include <stdio.h>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <thread>



// This function or variable may be unsafe. Consider using sprintf_s instead.
#pragma warning(disable : 4996)




#ifdef _WIN32

/* Windows-specific includes and setup */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // Link with Winsock library

#else

/* POSIX (Linux/macOS) includes and definitions */
#define __declspec(v)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
typedef int SOCKET;           // Normalize socket type
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef unsigned long DWORD;

#endif


bool init_sockets();
void cleanup_sockets();
void close_socket(SOCKET s);



#include "logger.hpp"
