#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>


#include "Json.h"
using json = nlohmann::json;

#include "logger.hpp"

#include "jmpdll.h"
#include "jmp_message.hpp"

#include "connection_status.hpp"


typedef int(*CallbackFunction)(char* sessionId);


class JniorJmp
{

private:
	CallbackFunction ConnectionCallback;
	CallbackFunction AuthenticationCallback;
	CallbackFunction MonitorCallback;

	char* m_uuid = new char[8];
	char* m_ipAddress;
	SOCKET m_sckt;

	DWORD m_dwThreadId;

	std::mutex mtx;
	std::condition_variable cv;
	bool dataReady = false;
	json responseJson;

	std::string _nonce;

	bool _loggedIn = false;
	int _loginFailureCount = 0;

	ConnectionStatus _connectionStatus;
	int _authenticationStatus = AUTHENTICATION_STATUS_ENUM::NOT_AUTHENTICATED;

public:
	bool b_quit;
	Logger* logfile;

	long ioTimestamp;
	json inputsJson;
	json outputsJson;

public:
	/**
	 * @brief	The JMP protocol constructor.  This initiates a connection to the given ip address
	 *			and the given JMP port.  The JMP port defaults to 9220.
	 *
	 * @param	ipAddress	the target JNIOR ip address
	 * @param	port		the target JNIOR port number used for the JMP protocol
	 */
	JniorJmp(const char* ipAddress, int port = 9220);
	~JniorJmp();

	void SetConnectionCallback(CallbackFunction callback) {
		ConnectionCallback = callback;
	}

	void SetAuthenticationCallback(CallbackFunction callback) {
		AuthenticationCallback = callback;
	}

	void SetMonitorCallback(CallbackFunction callback) {
		MonitorCallback = callback;
	}

	char* getUUID();

	SOCKET getSocket();
	int Connect();
	int Send(const char* jsonMessage);
	void MessageReceived(json json_obj);

	/**
	 * @brief		Sends the given credentials with the nonce that was received from the
	 *				authentication fialed message.
	 */
	int SendLogin(std::string username, std::string password);
	bool IsLoggedIn() {
		return _authenticationStatus == AUTHENTICATION_STATUS_ENUM::ADMINISTRATOR
			|| _authenticationStatus == AUTHENTICATION_STATUS_ENUM::USER
			|| _authenticationStatus == AUTHENTICATION_STATUS_ENUM::GUEST;
	}

	int GetInputs();
	int GetInput(int channelNumber);
	int GetOutputs();
	int GetOutput(int channelNumber);

	void ControlOutput(std::string command, int channelNumber);

	json ReadRegistryKeys(const std::vector<std::string>& registryKeys);

	json enumerateDevices();



	/**
	 * @brief		Reads a device
	 *
	 *			used to read data from an external module.  this is a generic method that will
	 *			just return the device data.  it is up to the calling method to decide if the
	 *			data is valid and to interpret the data.
	 *
	 * @param		deviceId		The ID of the device we should read
	 */
	json ReadDevice(std::string deviceId);
	int WriteDevice(std::string deviceId, std::string hexOutput);



	CallbackFunction getConnectionCallback() {
		return this->ConnectionCallback;
	}



	/**
	 * @brief		Returns the connection status object
	 */
	ConnectionStatus GetConnectionStatus() {
		return _connectionStatus;
	}



	//int GetConnectionStatus();
	//std::string GetConnectionStatusDescription();

	//void setConnectionStatus(CONNECTION_STATUS_ENUM connectionStatus) {

	//	// save the connection status and then alert the listener
	//	_connectionStatus = connectionStatus;

	//	if (nullptr != this->ConnectionCallback) {
	//		this->ConnectionCallback(this->m_uuid);
	//	}
	//}


	int GetAuthenticationStatus() {
		return this->_authenticationStatus;
	}

	std::string GetAuthenticationStatusDescription() {
		switch (this->_authenticationStatus) {
		case AUTHENTICATION_STATUS_ENUM::NOT_AUTHENTICATED:
			return "not authenticated";

		case AUTHENTICATION_STATUS_ENUM::AUTHENTICATING:
			return "authenticating...";

		case AUTHENTICATION_STATUS_ENUM::ADMINISTRATOR:
			return "authenticated as administrator";

		case AUTHENTICATION_STATUS_ENUM::USER:
			return "authenticated as user";

		case AUTHENTICATION_STATUS_ENUM::GUEST:
			return "authenticated as guest";

		default:
			return "UNKNOWN";
		}
	}

	void setAuthenticationStatus(AUTHENTICATION_STATUS_ENUM authenticationStatus) {

		// save the status and alert the listener
		_authenticationStatus = authenticationStatus;

		if (nullptr != this->AuthenticationCallback) {
			this->AuthenticationCallback(this->m_uuid);
		}
	}

private:
	void SendLogin(std::string username, std::string password, std::string nonce);

};



#include <cstring> // For strerror
#include <cerrno>  // For errno



static void receiverThread(void* lparam)
{
	JniorJmp* jniorJmp = (JniorJmp *)lparam;
	//SOCKET sckt = (SOCKET)lparam;
	SOCKET socket = jniorJmp->getSocket();

	jniorJmp->logfile->log("listener started");

	DWORD dwRecvTimeout = 2 * 60000; // 5 minutes
	int rc = setsockopt(socket,
		SOL_SOCKET, SO_RCVTIMEO,
		(const char*)&dwRecvTimeout, sizeof(dwRecvTimeout));

	try {
		while (!jniorJmp->b_quit) {

			char buffer[1024];

			// look for [
			while (true) {
				// read a byte and break if one was read
				int i = recv(socket, buffer, 1, 0);
				if (1 == i) break;

				// ZERO is peer initiated GRACEFUL close
				if (0 == i) {
					throw std::runtime_error("graceful close");
				}

				// less than ZERO means there was a socket error and we need to 
				//  figure out what it was.  it could be an error or it could be 
				//  a timeout that we use to initiate a keep-alive
				if (0 > i) {
					if (-1 == i) {
						jniorJmp->Send(JmpMessage().dump().c_str());
					}
					else {
						int i = -1;  // WSAGetLastError();
						throw std::runtime_error(std::string("Socket error: ") + strerror(errno) + strerror(i));
					}
				}
			}

			while ('[' != buffer[0]) {
				recv(socket, buffer, 1, 0);
			}

			// skip whitespace
			while (isspace(buffer[0])) {
				recv(socket, buffer, 1, 0);
			}

			// read and build a length string from the ascii digits
			int len = 0;
			recv(socket, buffer, 1, 0);
			do {
				// n is the numeric value if the ascii digit
				int n = buffer[0] - '0';
				len = len * 10 + n;
				recv(socket, buffer, 1, 0);
			} while (isdigit(buffer[0]));

			// skip whitespace
			while (isspace(buffer[0])) {
				recv(socket, buffer, 1, 0);
			}

			// read until , found
			while (',' != buffer[0]) {
				recv(socket, buffer, 1, 0);
			}

			// skip whitespace
			while (isspace(buffer[0])) {
				recv(socket, buffer, 1, 0);
			}

			// read the number of bytes we received from the length.  then copy 
			//  only the new data to a new array and null terminate it.
			int bytesRead = recv(socket, buffer, len, 0);
			char* jsonString = new char[bytesRead + 1];
			jsonString[bytesRead] = 0x0;
			strncpy(jsonString, buffer, bytesRead);

			// skip whitespace
			while (isspace(buffer[0])) {
				recv(socket, buffer, 1, 0);
			}

			// get the ending right square bracket
			while (']' != buffer[0]) {
				recv(socket, buffer, 1, 0);
			}

			json json_obj = json::parse(jsonString);
			jniorJmp->MessageReceived(json_obj);

			delete jsonString;
		}
	}
	catch (const std::runtime_error& e) {
		jniorJmp->logfile->error("error: " + std::string(e.what()));
	}

	close_socket(jniorJmp->getSocket());
	jniorJmp->logfile->log("listener done.");

	return;

}