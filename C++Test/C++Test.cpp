#include <iostream>

#include "../JmpDll/jmpdll.h"


int connection_callback(char* sessionId) {

	if (IsLoggedIn(sessionId)) {
		Log("connection to " + std::string(sessionId) + " was made");
	}
	else {
		Log("connection to " + std::string(sessionId) + " was NOT made or has been lost");
	}
	return 0;
}



int authentication_callback(char* sessionId) {
	bool is_logged_in = IsLoggedIn(sessionId);

	if (is_logged_in) {
		Log("authentication SUCCESS for " + std::string(sessionId));
	}
	else {
		Log("authentication failed for " + std::string(sessionId));
		Login(sessionId, "jnior", "jnior");
	}

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
