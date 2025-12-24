#include "stdafx.h"
#include "jmp_message.hpp"
#include "jnior_jmp.hpp"

#include "md5.h"

#include <cctype>
#include <sstream>
#include <typeinfo>


static DWORD WINAPI receiverThread(LPVOID *lparam);



JniorJmp::JniorJmp(const char* ipAddress, int port) {
	this->logfile = new Logger("JmpDll.log");

	// gerenate a uuid for this connection
	std::random_device rd;
	std::mt19937 engine(rd());
	std::uniform_int_distribution<int> dist(0, 0xffff);
	sprintf(this->m_uuid, "%04x%04x", dist(engine), dist(engine));

	// copy the target ip address
	this->m_ipAddress = new char[strlen(ipAddress)];
	strcpy(this->m_ipAddress, ipAddress);
}



JniorJmp::~JniorJmp() {
	//delete this->m_uuid;
	//delete this->m_ipAddress;

	this->b_quit = true;
}



int JniorJmp::SetConnectionCallback(CallbackFunction callback) {
	this->ConnectionCallback = callback;
	return 0;
}



int JniorJmp::SetAuthenticationCallback(CallbackFunction callback) {
	this->AuthenticationCallback = callback;
	return 0;
}



int JniorJmp::SetMonitorCallback(CallbackFunction callback) {
	this->MonitorCallback = callback;
	return 0;
}



//int JniorJmp::GetConnectionStatus() {
//	return this->_connectionStatus;
//}
//
//
//
//std::string JniorJmp::GetConnectionStatusDescription() {
//	switch (this->_connectionStatus) {
//	case CONNECTION_STATUS_ENUM::NOT_CONNECTED:
//		return "not connected";
//
//	case CONNECTION_STATUS_ENUM::CONNECTING:
//		return "connecting...";
//
//	case CONNECTION_STATUS_ENUM::CONNECTED:
//		return "connected";
//
//	case CONNECTION_STATUS_ENUM::CONNECTION_FAILED:
//		return "failed to connect";
//
//	case CONNECTION_STATUS_ENUM::CONNECTION_LOST:
//		return "connection lost";
//
//	default:
//		return "UNKNOWN";
//	}
//}



int JniorJmp::GetAuthenticationStatus() {
	return this->_authenticationStatus;
}



std::string JniorJmp::GetAuthenticationStatusDescription() {
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



SOCKET JniorJmp::getSocket() {
	return this->m_sckt;
}



int JniorJmp::Connect()
{
	sockaddr_in sck_add;

	// new socket
	this->m_sckt = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	// if we return INVALID_SOCKET then something went wrong.  call cleanup 
	// and raise the Error callback method
	if (m_sckt == INVALID_SOCKET) {
		WSACleanup();
	}

	this->_connectionStatus.setStatus(CONNECTION_STATUS_ENUM::CONNECTING);

	// everything looks good, continue to connect
	sck_add.sin_family = AF_INET;
	sck_add.sin_addr.s_addr = inet_addr(this->m_ipAddress);
	sck_add.sin_port = htons(9220);
	int connectResult = connect(this->m_sckt, (sockaddr*)&sck_add, sizeof(sockaddr_in));

	if (0 == connectResult) {
		// we were successfully connected.  start a receiver thread
		HANDLE m_hThread = CreateThread(
			NULL,										// default security attributes 
			0,											// use default stack size  
			(LPTHREAD_START_ROUTINE)receiverThread,  	// thread function 
			(LPVOID*)this,       						// argument to thread function 
			0,											// use default creation flags 
			&this->m_dwThreadId);						// returns the thread identifier 

		_loginFailureCount = 0;

		this->_connectionStatus.setStatus(CONNECTION_STATUS_ENUM::CONNECTED);

		// we were successfully connected.  send an empty message so that we get an 
		//  unauthenticated response with a Nonce to use in our login message
		this->Send(JmpMessage().dump().c_str());

	}
	else {
		this->_connectionStatus.setStatus(CONNECTION_STATUS_ENUM::CONNECTION_FAILED);

	}

	// we cant return until the connection has been negotiated
	this->dataReady = false;
	std::unique_lock<std::mutex> lock(this->mtx);
	cv.wait_for(lock, std::chrono::seconds(3), [this] { return dataReady; });

	this->logfile->log("connection negotiated");

	return connectResult;
}



void JniorJmp::MessageReceived(json json_obj) {

	this->logfile->log(this->m_ipAddress + std::string(" >>> ") + json_obj.dump());

	std::string message = json_obj["Message"];

	// is there a Meta object with a Hash value?
	std::string hash;
	if (json_obj.contains("Meta")) {
		hash = json_obj["Meta"]["Hash"];
	}

	if (message == "Error") {

		this->_nonce = json_obj["Nonce"];
		if (0 == this->_loginFailureCount++) {
			this->SendLogin("jnior", "jnior", this->_nonce);
			this->setAuthenticationStatus(AUTHENTICATION_STATUS_ENUM::AUTHENTICATING);
		}
		else {
			this->setAuthenticationStatus(AUTHENTICATION_STATUS_ENUM::AUTHENTICATION_FAILED);
		}

	}
	else if ("Authenticated" == message) {
		this->dataReady = true;
		this->responseJson = json_obj;

		_loggedIn = true;

		if (json_obj["Administrator"]) {
			this->setAuthenticationStatus(AUTHENTICATION_STATUS_ENUM::ADMINISTRATOR);
		}
		else {
			if (json_obj["Control"]) {
				this->setAuthenticationStatus(AUTHENTICATION_STATUS_ENUM::USER);
			}
			else {
				this->setAuthenticationStatus(AUTHENTICATION_STATUS_ENUM::GUEST);
			}
		}

		std::unique_lock<std::mutex> lock(mtx);
		lock.unlock();
		cv.notify_one();

	}
	else if ("Monitor" == message) {
		this->ioTimestamp = json_obj["Timestamp"];
		this->inputsJson = json_obj["Inputs"];
		this->outputsJson = json_obj["Outputs"];

		// if we have an monitor callback then call it.  the user will have to call the 
		//  appropriate IO methods to get the status of the IO.
		if (nullptr != this->MonitorCallback) {
			this->MonitorCallback(this->m_uuid);
		}

	}

	// check to see if a hash was found.  if it was then alert our conditional lock that is 
	//  waiting on the response
	if (!hash.empty()) {
		this->dataReady = true;
		this->responseJson = json_obj;

		std::unique_lock<std::mutex> lock(mtx);
		lock.unlock();
		cv.notify_one();
	}

}



int JniorJmp::SendLogin(std::string username, std::string password) {
	this->SendLogin(username, password, this->_nonce);
	return 0;
}



void JniorJmp::SendLogin(std::string username, std::string password, std::string nonce) {

	// build a string to get the digest from
	std::stringstream ss;
	ss << username << ":" << nonce << ":" << password;
	void* md5 = hashing::md5::hash(ss.str());
	const char* authDigest = hashing::md5::sig2hex(md5).c_str();

	char* loginMessage = new char[64];
	sprintf(loginMessage, "{\"Auth-Digest\": \"%s:%s\"}", username.c_str(), hashing::md5::sig2hex(md5).c_str());

	this->Send((char*)loginMessage);

	delete loginMessage;
	delete md5;
}



bool JniorJmp::IsLoggedIn() {
	return this->_loggedIn;
}



int JniorJmp::GetInputs() {
	return 0;
}



int JniorJmp::GetOutputs() {
	return 0;
}



int JniorJmp::GetInput(int channelNumber) {

	if (this->outputsJson.size() < channelNumber) {
		return CHANNEL_OUT_OF_RANGE;
	}

	return this->inputsJson[channelNumber - 1]["State"];
}



int JniorJmp::GetOutput(int channelNumber) {

	if (this->outputsJson.size() < channelNumber) {
		return CHANNEL_OUT_OF_RANGE;
	}

	return this->outputsJson[channelNumber - 1]["State"];
}



void JniorJmp::ControlOutput(std::string command, int channelNumber) {
	this->Send((char *)(ControlOutputMessage(command, channelNumber).dump().c_str()));
}



json JniorJmp::ReadRegistryKeys(const std::vector<std::string>& registryKeys) {
	this->Send((char *)(ReadRegistryMessage(registryKeys).dump().c_str()));

	this->dataReady = false;
	std::unique_lock<std::mutex> lock(this->mtx);
	cv.wait(lock, [this] { return dataReady; });

	json json_response = this->responseJson;
	return json_response["Keys"];
}



json JniorJmp::enumerateDevices() {
	this->Send(EnumerateDevicesMessage().dump().c_str());

	this->dataReady = false;
	std::unique_lock<std::mutex> lock(this->mtx);
	cv.wait(lock, [this] { return dataReady; });

	json json_obj = this->responseJson;
	json devicesJsonArray = json_obj["Devices"];

	return devicesJsonArray;
}



json JniorJmp::ReadDevice(std::string deviceId) {
	this->Send(ReadDevicesMessage(deviceId).dump().c_str());

	this->dataReady = false;
	std::unique_lock<std::mutex> lock(this->mtx);
	cv.wait(lock, [this] { return dataReady; });

	json json_obj = this->responseJson;
	json devicesJsonArray = json_obj["Devices"];

	return devicesJsonArray;
}



int JniorJmp::WriteDevice(std::string deviceId, std::string hexOutput) {
	this->Send(WriteDevicesMessage(deviceId, hexOutput).dump().c_str());

	this->dataReady = false;
	std::unique_lock<std::mutex> lock(this->mtx);
	cv.wait(lock, [this] { return dataReady; });

	json json_obj = this->responseJson;
	json devicesJsonArray = json_obj["Devices"];
	json json1 = devicesJsonArray[0];
	std::cout << json1.dump() << std::endl;
	bool result = json1.at("Result");
	if (result) {
		return 0;
	}
	return -1;
}



int JniorJmp::Send(const char* jsonMessage) {

	this->logfile->log(this->m_ipAddress + std::string(" <<< ") + jsonMessage);

	// get the length of the message that we want to send.  then build the byte 
	//  buffer large enough for the message plus the square brackets, length 
	//  field, and the comma
	int messageLen = strlen(jsonMessage);
	char* bytes = new char[messageLen + 7];

	sprintf(bytes, "[%d ,%s]", messageLen, jsonMessage);
	int sentBytes = send(this->m_sckt, bytes, strlen(bytes), 0);

	//delete bytes;

	// return the number of bytes that were sent
	return sentBytes;
}



char* JniorJmp::getUUID() {
	return this->m_uuid;
}



#include <cstring> // For strerror
#include <cerrno>  // For errno



static DWORD WINAPI receiverThread(LPVOID *lparam)
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
						int i = WSAGetLastError();
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

	// set the connection status to CLOSED
	jniorJmp->GetConnectionStatus().setStatus(CONNECTION_STATUS_ENUM::CONNECTION_LOST);

	jniorJmp->logfile->log("listener done.");

	return 0;

}