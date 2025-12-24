#include "stdafx.h"
#include "jnior_jmp.hpp"

#include "md5.h"

#include <cctype>
#include <sstream>
#include <typeinfo>


// void receiverThread(void* lparam);



JniorJmp::JniorJmp(const char* ipAddress, int port) {
	this->logfile = new Logger("JmpDll.log");

	// gerenate a uuid for this connection
	std::random_device rd;
	std::mt19937 engine(rd());
	std::uniform_int_distribution<int> dist(0, 0xffff);
	sprintf(this->m_uuid, "%08x", dist(engine));

	// copy the target ip address
	this->m_ipAddress = new char[strlen(ipAddress)];
	strcpy(this->m_ipAddress, ipAddress);
}



JniorJmp::~JniorJmp() {
	//delete this->m_uuid;
	//delete this->m_ipAddress;

	this->b_quit = true;
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
		cleanup_sockets();
	}

	// everything looks good, continue to connect
	sck_add.sin_family = AF_INET;
	sck_add.sin_addr.s_addr = inet_addr(this->m_ipAddress);
	sck_add.sin_port = htons(9220);

	// try to connect
	int connectResult = connect(this->m_sckt, (sockaddr *)&sck_add, sizeof(sockaddr_in));

	if (0 == connectResult) {
		// we were successfully connected.  start a receiver thread

		_receiverThread = std::thread(receiverThread, (void*)this);

		//unsigned long m_dwThreadId;
		//HANDLE m_hThread = CreateThread(
		//	NULL,										// default security attributes 
		//	0,											// use default stack size  
		//	(LPTHREAD_START_ROUTINE)receiverThread,  	// thread function 
		//	(LPVOID*)this,       						// argument to thread function 
		//	0,											// use default creation flags 
		//	&m_dwThreadId);								// returns the thread identifier 

		// we were successfully connected.  send an empty message so that we get an 
		//  unauthenticated response with a Nonce to use in our login message
		this->Send(JmpMessage().dump().c_str());
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
	std::cout << message << std::endl;

	// is there a Meta object with a Hash value?
	std::string hash;
	if (json_obj.contains("Meta")) {
		hash = json_obj["Meta"]["Hash"];
	}

	if (message == "Error") {
		std::string nonce = json_obj["Nonce"];
		this->SendLogin("jnior", "jnior", nonce);

	}
	else if ("Authenticated" == message) {
		this->dataReady = true;
		this->responseJson = json_obj;

		std::unique_lock<std::mutex> lock(mtx);
		lock.unlock();
		cv.notify_one();

	}
	else if ("Monitor" == message) {
		this->ioTimestamp = json_obj["Timestamp"];
		this->inputsJson = json_obj["Inputs"];
		this->outputsJson = json_obj["Outputs"];

	}

	// check to see if a hash was found.  if it was then alert our conditional 
	//  lock that is waiting on the response
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

	char loginMessage[64];
	sprintf(loginMessage, "{\"Auth-Digest\": \"%s:%s\"}", username.c_str(), hashing::md5::sig2hex(md5).c_str());

	this->Send((char*)loginMessage);
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
