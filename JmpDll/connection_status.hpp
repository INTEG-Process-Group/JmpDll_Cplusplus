#pragma once

#include <string>

#include "jmpdll.h"
#include "status.hpp"


class ConnectionStatus :
	public Status
{

public:
	ConnectionStatus() : Status(CONNECTION_STATUS_ENUM::NOT_CONNECTED) { }

	std::string GetConnectionStatusDescription() {
		switch (this->_status) {
		case CONNECTION_STATUS_ENUM::NOT_CONNECTED:
			return "not connected";

		case CONNECTION_STATUS_ENUM::CONNECTING:
			return "connecting...";

		case CONNECTION_STATUS_ENUM::CONNECTED:
			return "connected";

		case CONNECTION_STATUS_ENUM::CONNECTION_FAILED:
			return "failed to connect";

		case CONNECTION_STATUS_ENUM::CONNECTION_LOST:
			return "connection lost";

		default:
			return "UNKNOWN";
		}
	}

};

