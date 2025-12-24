#include <iostream>
#include <string>

#include "../../JmpDll/jmpdll.h"


int connection_callback(char* sessionId) {
	Log(std::string(sessionId) + " is " + GetConnectionStatusDescription(sessionId));
	return 0;
}



int authentication_callback(char* sessionId) {
	Log(std::string(sessionId) + " is " + GetAuthenticationStatusDescription(sessionId));
	return 0;
}



int monitor_callback(char* sessionId) {
	Log("monitor packet received for " + std::string(sessionId));
	return 0;
}



int main()
{

	// we should be able to assign a callback to get alerted when a connection is made
	SetConnectionCallback(connection_callback);
	SetAuthenticationCallback(authentication_callback);
	SetMonitorCallback(monitor_callback);

	// create a connection and save the sessionid
	char sessionId[32];
	int result = CreateConnection("10.0.0.62", sessionId);

	int read;
	std::cin >> read;
}
