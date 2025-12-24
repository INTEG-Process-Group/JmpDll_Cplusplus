#pragma once

#include <condition_variable>
#include <mutex>

#include "Json.h"
using json = nlohmann::json;

#include "logger.hpp"

#include "jmpdll.h"

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

	int SetConnectionCallback(CallbackFunction callback);
	int SetAuthenticationCallback(CallbackFunction callback);
	int SetMonitorCallback(CallbackFunction callback);

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
	bool IsLoggedIn();

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


	int GetAuthenticationStatus();
	std::string GetAuthenticationStatusDescription();

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

