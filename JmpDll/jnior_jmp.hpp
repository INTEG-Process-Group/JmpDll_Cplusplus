#pragma once

#include <condition_variable>
#include <mutex>

#include "Json.h"
using json = nlohmann::json;

#include "logger.hpp"


class JniorJmp
{

private:
	char* m_uuid = new char[8];
	char* m_ipAddress;
	SOCKET m_sckt;

	std::mutex mtx;
	std::condition_variable cv;
	bool dataReady = false;
	json responseJson;

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

	char* getUUID();

	SOCKET getSocket();
	int Connect();
	int Send(const char* jsonMessage);
	void MessageReceived(json json_obj);

	void SendLogin(std::string username, std::string password, std::string nonce);

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
};

