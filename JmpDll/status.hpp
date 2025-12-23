#pragma once

class Status
{
protected:
	int _status;

	Status(int status) {
		setStatus(status);
	}

public:
	int getStatus() {
		return _status;
	};


	void setStatus(int status) {
		_status = status;
	}

};

