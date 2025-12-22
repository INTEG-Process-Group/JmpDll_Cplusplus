#pragma once

#include <random>

#include "Json.h"
using namespace nlohmann;



// This function or variable may be unsafe. Consider using sprintf_s instead.
#pragma warning(disable : 4996)




class JmpMessage
{
protected:
	json json_obj;

	JmpMessage(std::string message) {
		json_obj["Message"] = message;
	}

public:
	JmpMessage() {
		json_obj["ACK"] = "";
	}

	std::string dump() {
		return json_obj.dump();
	}
};



//JmpMessage EmptyMessage("");



class MetaHashMessage : public JmpMessage
{
public:
	MetaHashMessage(std::string message) : JmpMessage(message) {
		std::random_device rd;
		std::mt19937 engine(rd());
		std::uniform_int_distribution<int> dist(0, 0xffff);

		char* hash = new char[8];
		sprintf(hash, "%04x%04x", dist(engine), dist(engine));
		json_obj["Meta"] = json::object({ {"Hash", hash} });
	}
};


class ControlOutputMessage : public JmpMessage
{
public:
	ControlOutputMessage(std::string command, int channelNumber) : JmpMessage("Control") {
		json_obj["Command"] = command;
		json_obj["Channel"] = channelNumber;
	}
};



class ReadRegistryMessage : public MetaHashMessage
{
public:
	ReadRegistryMessage(const std::vector<std::string>& registryKeys) : MetaHashMessage("Registry Read") {
		json_obj["Keys"] = json::array();

		for (const std::string& registryKey : registryKeys) {
			json_obj["Keys"].push_back(registryKey);
		}
	}
};



/****** External Devices *****************************************************/

class EnumerateDevicesMessage : public MetaHashMessage
{
public:
	EnumerateDevicesMessage() : MetaHashMessage("Enumerate Devices") { }
};



class ReadDevicesMessage : public MetaHashMessage
{
public:
	ReadDevicesMessage(std::string deviceId) : MetaHashMessage("Read Devices") {
		json_obj["Devices"] = json::array({ deviceId });
	}
};



class WriteDevicesMessage : public MetaHashMessage
{
public:
	WriteDevicesMessage(std::string deviceId, std::string hex) : MetaHashMessage("Write Devices") {
		json_obj["Devices"] = json::array();
		json_obj["Devices"].push_back(json::object({ { "Address", deviceId}, {"Hex", hex } }));
	}
};
