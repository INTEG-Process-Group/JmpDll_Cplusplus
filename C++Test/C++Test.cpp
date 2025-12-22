#include <iostream>

#include "../JmpDll/jmpdll.h"


int connection_made(char* sessionId) {
	std::cout << "connection to " << sessionId << " was made " << std::endl;
	return 0;
}



int authentication_failed(char* sessionId) {
	std::cout << "authentication failed for " << sessionId << std::endl;

	Login(sessionId, "jnior", "jnior");

	return 0;
}



int authenticated(char* sessionId) {
	std::cout << "connection to " << sessionId << " has been authenticated " << std::endl;
	return 0;
}



int main()
{

	// we should be able to assign a callback to get alerted when a connection is made
	SetConnectionCallback(connection_made);
	SetAuthenticationFailedCallback(authentication_failed);
	SetAuthenticatedCallback(authenticated);

	// create a connection and save the sessionid
	char sessionId[32];
	int result = CreateConnection("10.0.0.62", sessionId);
	std::cout << sessionId << std::endl;

	int read;
	std::cin >> read;
}
