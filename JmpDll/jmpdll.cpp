// dllmain.cpp : Defines the entry point for the DLL application.

#pragma once

#include <iterator>

// for double NaN
#include <limits>

#include "pch.h"
#include "stdafx.h"

#include "jnior_jmp.hpp"

#include "Json.h"
using json = nlohmann::json;

#include "hex_utils.hpp"

#include "jmpdll.h"


#pragma comment(lib,"WS2_32") 



// This function or variable may be unsafe. Consider using sprintf_s instead.
#pragma warning(disable : 4996)



std::map<std::string, JniorJmp*> jnior_connections;
std::map<std::string, TEN_VOLT*> ten_volt_devices_by_id;


// winsock data
WSADATA wd;

std::mutex mtx;


#include "Logger.hpp"
Logger logfile("jmpdll.log");



BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		logfile.log("Process Attach");

		// dumps memory leaks to the debug window
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		// Startup Winsock
		logfile.log("Start up Winsock");
		if (WSAStartup(MAKEWORD(2, 2), &wd) != NO_ERROR)
		{
			logfile.error("Error at WSAStartup()\n");
			return false;
		}

		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		logfile.log("Process Detach");

		//// Iterate through the map
		//for (const auto& pair : jnior_connections) {
		//	cout << pair.first << " => " << pair.second << std::endl;
		//	delete ((JniorJmp*)(pair.second));
		//}

		//WSACleanup();
		break;
	}

	return TRUE;
}



/**
 * @brief  Validates a Connection UUID by making sure it not blank and that it exists in our
 *         dictionary.  This indicates that there is a connection available for this UUID.
 *
 * @param  connectionUUID  A string representing a connection
 * @return  boolean indicating if the connectionUUID exists
 */
bool validate_uuid(const char* connectionUUID) {
	return (0 < strlen(connectionUUID) && jnior_connections.end() != jnior_connections.find(connectionUUID));
}



int validate_device_type(const std::string deviceId, int deviceType) {
	// validate a device id was givena and the length of the device id
	if (16 != deviceId.length()) return INVALID_DEVICE_ID;

	try {
		// validate the device type
		long long deviceAddress = std::stoull(deviceId, nullptr, 16);
		if (deviceType != (deviceAddress & 0xff)) {
			return WRONG_DEVICE_TYPE;
		}
	}
	catch (const std::out_of_range& oor) {
		return INVALID_DEVICE_ID;
	}
	catch (const std::invalid_argument& ia) {
		return INVALID_DEVICE_ID;
	}

	return OK;
}



JMPDLL_API int GetDllVersion(char* versionString) {
	sprintf(versionString, "%d.%d.%d", 25, 1, 23);
	return 0;
}



JMPDLL_API int CreateConnection(const char* ipAddress, char* connectionUUID) {

	logfile.log(std::string("get connection for ip address: ") + std::string(ipAddress));

	// make sure there was a valid ip address provided
	if (0 == strlen(ipAddress)) return INVALID_IP_ADDRESS;

	// create a JniorJmp instance and assign it to our connections dictionary
	JniorJmp* jniorJmp = new JniorJmp(ipAddress);

	// assign the new connection uuid to the variable pointer that was provided
	strcpy(connectionUUID, jniorJmp->getUUID());
	logfile.log(std::string("assign connection uuid: ") + std::string(connectionUUID));

	// add the connection to our table
	jnior_connections[connectionUUID] = jniorJmp;

	// try to connect
	int connect = jniorJmp->Connect();
	return connect;

}



JMPDLL_API int GetInput(const char* connectionUUID, int channelNumber) {

	// make sure the provided connection uuid is valid and is found in the connection map.  if it 
	//  is valid then continue and get the jmp connection object from our dictionary.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	return jniorJmp->GetInput(channelNumber);
}



JMPDLL_API int GetOutput(const char* connectionUUID, int channelNumber) {

	// make sure the provided connection uuid is valid and is found in the connection map.  if it 
	//  is valid then continue and get the jmp connection object from our dictionary.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	return jniorJmp->GetOutput(channelNumber);
}



JMPDLL_API int ControlOutput(const char* connectionUUID, const char* command, int channelNumber, int duration = -1) {

	// make sure the provided connection uuid is valid and is found in the connection map.  if it 
	//  is valid then continue and get the jmp connection object from our dictionary.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	jniorJmp->ControlOutput(command, channelNumber);

	return 0;
}



JMPDLL_API int CloseOutput(const char* connectionUUID, int channelNumber) {
	return ControlOutput(connectionUUID, "Close", channelNumber);
}



JMPDLL_API int OpenOutput(const char* connectionUUID, int channelNumber) {
	return ControlOutput(connectionUUID, "Open", channelNumber);
}



JMPDLL_API int ToggleOutput(const char* connectionUUID, int channelNumber) {
	return ControlOutput(connectionUUID, "Toggle", channelNumber);
}



JMPDLL_API int ClosePulseOutput(const char* connectionUUID, int channelNumber, int duration) {
	return ControlOutput(connectionUUID, "Close Pulse", channelNumber, duration);
}



JMPDLL_API int OpenPulseOutput(const char* connectionUUID, int channelNumber, int duration) {
	return ControlOutput(connectionUUID, "Open Pulse", channelNumber, duration);
}



JMPDLL_API int ReadRegistryKeys(const char* connectionUUID, REGISTRY_KEY* registryKeys, int keyCount) {

	// make sure the provided connection uuid is valid and is found in the connection map.  if it 
	//  is valid then continue and get the jmp connection object from our dictionary.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	// build a vector with the key names that we want to read
	std::vector<std::string> registryKeyVector;
	for (int i = 0; i < keyCount; i++) {
		registryKeyVector.push_back(registryKeys[i].keyName);
	}

	// call the read registry keys method on our jmp connection
	json keysJson = jniorJmp->ReadRegistryKeys(registryKeyVector);

	// assign the values from the response to our structures
	for (int i = 0; i < keyCount; i++) {
		if (!keysJson[registryKeys[i].keyName].is_null()) {
			std::string value = keysJson[registryKeys[i].keyName];
			registryKeys[i].value = new char[value.length() + 1];
			strncpy(registryKeys[i].value, value.c_str(), value.length());
			registryKeys[i].value[value.length()] = '\0';
		}
		else {
			registryKeys[i].value = new char[1];
			registryKeys[i].value[0] = '\0';
		}
	}

	return 0;
}



JMPDLL_API int EnumerateDevices(const char* connectionUUID, char** connectedDevices) {

	// make sure the provided connection uuid is valid and is found in the connection map.  if it 
	//  is valid then continue and get the jmp connection object from our dictionary.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	json devicesJson = jniorJmp->enumerateDevices();
	for (int i = 0; i < devicesJson.size(); i++) {
		std::string deviceId = devicesJson[i];
		if (nullptr != connectedDevices[i]) {
			strcpy(connectedDevices[i], deviceId.c_str());
		}
	}

	return OK;
}



JMPDLL_API int GetTemperature(const char* connectionUUID, const char* deviceId, TEMPERATURE* temp_struct) {

	// make sure the provided connection uuid is valid and is found in the connection map.  if it 
	//  is valid then continue and get the jmp connection object from our dictionary.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	// validate the deviceId
	int deviceTypeCheck = validate_device_type(deviceId, 0x28);
	if (OK != deviceTypeCheck) return deviceTypeCheck;

	for (int i = 0; i < 3; i++) {

		if (0 < i) {
			logfile.warn("retrying read");
		}

		json devicesJsonArray = jniorJmp->ReadDevice(deviceId);

		std::string deviceHexValue = devicesJsonArray[0]["Hex"];
		if (0 < deviceHexValue.length()) {
			temp_struct->tempC = hex_to_double(deviceHexValue);
			temp_struct->tempF = temp_struct->tempC * 9 / 5 + 32;
			break;
		}
	}

	return 0;
}



JMPDLL_API int GetTemperatureByChannel(const char* connectionUUID, int channel, TEMPERATURE* temp_struct) {

	// get the device needed for the temp probe
	std::string deviceOrderRegistryString = "externals/deviceorder/type28_" + std::to_string(channel);
	REGISTRY_KEY deviceOrderRegistryKey;
	deviceOrderRegistryKey.keyName = deviceOrderRegistryString.c_str();
	REGISTRY_KEY registryKeys[1];
	registryKeys[0] = deviceOrderRegistryKey;
	int readResult = ReadRegistryKeys(connectionUUID, registryKeys, 1);
	std::string deviceId = registryKeys[0].value;

	if (nullptr == registryKeys[0].value) return MODULE_DOES_NOT_EXIST;
	if (0 == deviceId.length()) return MODULE_DOES_NOT_EXIST;

	return GetTemperature(connectionUUID, deviceId.c_str(), temp_struct);
}



// $$$$ TODO:  should this be in the jnior_jmp class?  probably
int ReadDevice(const char* connectionUUID, const std::string deviceId, char* deviceResponse) {

	// make sure the provided connection uuid is valid and is found in the connection map.  if it 
	//  is valid then continue and get the jmp connection object from our dictionary.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	// perform up to three read attempts.  one wire reads can fail so this loop allows for two retries
	for (int i = 0; i < 3; i++) {

		// log if this is a retry
		if (0 < i) logfile.warn("retrying read to " + std::string(deviceId));

		// perform the read and get the json array response.
		// $$$$ TODO - here we only processing one device.  we need to handle more.
		json devicesJsonArray = jniorJmp->ReadDevice(deviceId);

		std::string deviceHexValue = devicesJsonArray[0]["Hex"];
		if (0 < deviceHexValue.length()) {
			strncpy(deviceResponse, deviceHexValue.c_str(), deviceHexValue.length());
			return OK;
		}
	}

	return -1;
}



JMPDLL_API int GetEnviron(const char* connectionUUID, const char* deviceId, ENVIRON* environ_struct) {

	// make sure the provided connection uuid is valid and is found in the connection map.
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;

	// validate the deviceId
	int deviceTypeCheck = validate_device_type(deviceId, 0x7e);
	if (OK != deviceTypeCheck) return deviceTypeCheck;

	// validate the ENVIRON
	if (nullptr == environ_struct) return INVALID_ARGUMENT;

	char* deviceResponse = new char[32];
	int readDeviceResult = ReadDevice(connectionUUID, deviceId, deviceResponse);
	if (OK == readDeviceResult) {
		std::string deviceHexValue = std::string(deviceResponse);

		short tempC, humidity;
		std::sscanf(deviceHexValue.substr(4, 4).c_str(), "%hx", &tempC);
		std::sscanf(deviceHexValue.substr(8, 4).c_str(), "%hx", &humidity);

		// update the environmental structure
		if (0xffff == tempC) logfile.warn("temp c out of range");
		else {
			environ_struct->tempC = tempC / 16.0;
			environ_struct->tempF = environ_struct->tempC * 9 / 5 + 32;
		}

		if (0xffff == humidity) logfile.warn("humidity out of range");
		else environ_struct->humidity = humidity / 16.0;
	}

	// cleanup the temporary device response and return our result
	delete deviceResponse;
	return readDeviceResult;

}



JMPDLL_API int GetTenVolt(const char* connectionUUID, const char* deviceId, TEN_VOLT* ten_volt_struct) {

	// validate the deviceId
	int deviceTypeCheck = validate_device_type(deviceId, TEN_VOLT_MODULE_ID);
	if (OK != deviceTypeCheck) return deviceTypeCheck;

	// validate the TEN_VOLT
	if (nullptr == ten_volt_struct) return INVALID_ARGUMENT;

	char* deviceResponse = new char[32];
	int readDeviceResult = ReadDevice(connectionUUID, std::string(deviceId), deviceResponse);
	if (OK == readDeviceResult) {
		std::string deviceHexValue = std::string(deviceResponse);

		std::sscanf(deviceHexValue.substr(0, 4).c_str(), "%hx", &ten_volt_struct->rawIns[0]);
		std::sscanf(deviceHexValue.substr(4, 4).c_str(), "%hx", &ten_volt_struct->rawIns[1]);
		std::sscanf(deviceHexValue.substr(8, 4).c_str(), "%hx", &ten_volt_struct->rawIns[2]);
		std::sscanf(deviceHexValue.substr(12, 4).c_str(), "%hx", &ten_volt_struct->rawIns[3]);

		std::sscanf(deviceHexValue.substr(16, 4).c_str(), "%hx", &ten_volt_struct->rawOuts[0]);
		std::sscanf(deviceHexValue.substr(20, 4).c_str(), "%hx", &ten_volt_struct->rawOuts[1]);
	}

	delete deviceResponse;
	return readDeviceResult;

}



int WriteDevice(const char* connectionUUID, const std::string deviceId, const std::string writeHex) {
	// make sure the provided connection uuid is valid and is found in the connection map
	if (!validate_uuid(connectionUUID)) return INVALID_UUID;

	// get the connection from the UUID
	JniorJmp* jniorJmp = jnior_connections[connectionUUID];

	// perform up to three read attempts.  one wire reads can fail so this loop allows for two retries
	for (int i = 0; i < 3; i++) {

		// log if this is a retry
		if (0 < i) logfile.warn("retrying read to " + std::string(deviceId));

		// perform the write
		// $$$$ TODO - here we only processing one device.  we need to handle more.
		if (OK == jniorJmp->WriteDevice(deviceId, writeHex)) return OK;
	}

	return -1;
}



JMPDLL_API int SetTenVolt(const char* connectionUUID, const std::string deviceId, const TEN_VOLT* ten_volt_struct) {

	// validate the deviceId
	int deviceTypeCheck = validate_device_type(deviceId, TEN_VOLT_MODULE_ID);
	if (OK != deviceTypeCheck) return deviceTypeCheck;

	// validate the TEN_VOLT
	if (nullptr == ten_volt_struct) return INVALID_ARGUMENT;

	std::stringstream ss;
	// Use std::hex manipulator to set the base to hexadecimal
	ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << ten_volt_struct->rawOuts[0];
	ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << ten_volt_struct->rawOuts[1];

	int readDeviceResult = WriteDevice(connectionUUID, deviceId, ss.str());
	if (OK == readDeviceResult) {
	}

	return readDeviceResult;

}



JMPDLL_API int SetTenVoltChannelPercentage(const char* connectionUUID, int channel, double percentage) {

	// get the device needed for the 10v output channel
	int moduleNumber = (channel - 1) / 2 + 1;
	std::string deviceOrderRegistryString = "externals/deviceorder/typeFD_" + std::to_string(moduleNumber);
	REGISTRY_KEY deviceOrderRegistryKey;
	deviceOrderRegistryKey.keyName = deviceOrderRegistryString.c_str();
	REGISTRY_KEY registryKeys[1];
	registryKeys[0] = deviceOrderRegistryKey;
	int readResult = ReadRegistryKeys(connectionUUID, registryKeys, 1);
	std::string deviceId = registryKeys[0].value;

	if (nullptr == registryKeys[0].value) return MODULE_DOES_NOT_EXIST;
	if (0 == deviceId.length()) return MODULE_DOES_NOT_EXIST;

	// validate the percentage range
	if (0 > percentage || 100 < percentage) return INVALID_ARGUMENT;

	// get the 10v structure for the device.  if it does not exist then we must 
	//  create it and perform an initial read
	if (ten_volt_devices_by_id.end() == ten_volt_devices_by_id.find(deviceId)) {
		ten_volt_devices_by_id[deviceId] = new TEN_VOLT();
		GetTenVolt(connectionUUID, deviceId.c_str(), ten_volt_devices_by_id[deviceId]);
	}
	TEN_VOLT* ten_volt_struct = ten_volt_devices_by_id[deviceId];

	// modify the channel with the new value based on the given percentage
	ten_volt_struct->rawOuts[(channel - 1) % 2] = percentage / 100.0 * 65520;

	return SetTenVolt(connectionUUID, deviceId, ten_volt_struct);

}
