#pragma once

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JNIORDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JNIORDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef JMPDLL_API
#define JMPDLL_API __declspec(dllexport)



extern "C" {
	/***** RETURN CODES **********************************************************/
	JMPDLL_API int OK = 0;
	JMPDLL_API int INVALID_IP_ADDRESS = -101;
	JMPDLL_API int INVALID_UUID = -102;
	JMPDLL_API int INVALID_DEVICE_ID = -103;
	JMPDLL_API int WRONG_DEVICE_TYPE = -104;
	JMPDLL_API int INVALID_ARGUMENT = -105;
	JMPDLL_API int CHANNEL_OUT_OF_RANGE = -106;
	JMPDLL_API int MODULE_DOES_NOT_EXIST = -107;


	/***** ENTERNAL DEVICE TYPES *************************************************/
	JMPDLL_API int FOUR_TWENTY_MODULE_ID = 0xfe;
	JMPDLL_API int TEN_VOLT_MODULE_ID = 0xfd;
	//JMPDLL_API int RTD_MODULE_ID = 0xfc;
	JMPDLL_API int FOUR_ROUT_MODULE_ID = 0xfb;
	JMPDLL_API int CONTROL_PANEL_MODULE_ID = 0xfa;
	JMPDLL_API int THREE_CHANNEL_DIMMER_MODULE_ID = 0xf9;
	JMPDLL_API int OCTAL_INPUT_MODULE_ID = 0xf8;
	JMPDLL_API int ENVIRON_SENSOR_ID = 0x7e;


	/***** STRUCTURES ************************************************************/

	/*
	 * a structure representing a registry key along with its value
	 */
#pragma pack(1)
	JMPDLL_API struct REGISTRY_KEY {
		const char* keyName;
		char* value;
	};
#pragma pack(pop)

	/*
	 * a temperature structure
	 */
#pragma pack(1)
	JMPDLL_API struct TEMPERATURE {
		double tempC;
		double tempF;
	};
#pragma pack(pop)

	/*
	 * an environmental structure holding properties that belong to the
	 *  EDS Envornmental Sensors 0x7E
	 */
#pragma pack(1)
	JMPDLL_API struct ENVIRON {
		double tempC;
		double tempF;
		double humidity;
	};
#pragma pack(pop)

	/*
	 * INTEG Ten Volt - Type 0xFD
	 */
#pragma pack(1)
	JMPDLL_API struct TEN_VOLT {
		unsigned short rawIns[4];
		unsigned short rawOuts[2];
	};
#pragma pack(pop)



	/***** GLOBAL METHODS ********************************************************/

	/**
	 * @brief  Gets the compiled DLL version in the format YEAR.VERSION.BUILD
	 *
	 * @param  version  A pointer to a char array that will hold the version string
	 *
	 * @return OK for success
	 */
	JMPDLL_API int GetDllVersion(char* versionString);

	typedef int (*CallbackFunction)(char* sessionId);
	
	/**
	 * @brief		Sets a callback that is called when a connection state changes
	 *
	 * We must now call IsConnected() for the session to get the actual status of the connection.
	 *
	 * @param		callback		A funtion that will be called when the connection status changes
	 */
	JMPDLL_API int SetConnectionCallback(CallbackFunction callback);

	/**
	 * @brief		Sets a callback that is called when a authentication state changes
	 *
	 * We must now call IsLoggedIn() for the session to get the actual status of the authentication.
	 *
	 * @param		callback		A funtion that will be called when the authentication status changes
	 */
	JMPDLL_API int SetAuthenticationCallback(CallbackFunction callback);

	/**
	 * @brief		Sets a callback that is called when a monitor packet is received
	 *
	 * We must now call methods to get the state of the IO
	 *
	 * @param		callback		A funtion that will be called when sa monitor packet is received
	 */
	JMPDLL_API int SetMonitorCallback(CallbackFunction callback);


	/**
	 * @brief  Creates a connection to a JMP stream on the JNIOR with the given IP Address
	 *
	 *   creates a connection to the given IP Address.  Returns a unique UUID for the connection.
	 *    Every call to this method will make a new connection to the ip address and return a new
	 *    UUID.  This method waits for the new connection to be negotiated.  This code will send
	 *    the EMPTY MESSAGE to prompt the JNIOR to send the Authentication Error.
	 *
	 * @param  ipAddress  of the JNIOR to connect to
	 * @param  connectionUUID  a pointer to a string that will accept the connection UUID of the
	 *         established connection.  This value is only created if the connection was successful.
	 *
	 * @return OK if successful
	 */
	JMPDLL_API int CreateConnection(const char* ipAddress, char* uuid);

	JMPDLL_API int Connect(const char* connectionUUID);

	JMPDLL_API int Login(const char* connectionUUID, const char* username, const char* password);



	/***** PER CONNECTION METHODS ************************************************/

	/*****   CONNECTION STATUS   *************************************************/

	/**
	 * @brief		returns whether the connection referenced by the connectionUUID is currently 
	 *				connected to a JNIOR
	 *
	 * @param  ipAddress  of the JNIOR to get the connected status from
	 */
	JMPDLL_API bool IsConnected(const char* connectionUUID);

	/**
	 * @brief		returns whether the connection referenced by the connectionUUID is currently 
	 *				logged in to a JNIOR
	 *
	 * @param  ipAddress  of the JNIOR to get the logged in status from
	 */
	JMPDLL_API bool IsLoggedIn(const char* connectionUUID);


	/*****   INTERNAL IO   *******************************************************/

	/*
	 * @brief		gets the state of an INPUT identified by the channel number
	 */
	JMPDLL_API int GetInput(const char* connectionUUID, int channelNumber);

	/**
	 * @brief		gets the state of an Output identified by the channel number.  the channel 
	 *				numbers indexed starting with 1.
	 *
	 * @param	connectionUUID	a string representing a connection
	 * @param	channelNumber	the channel we wish to get the state of
	 *
	 * @return  0 - LOW, 1 - HIGH
	 */
	JMPDLL_API int GetOutput(const char* connectionUUID, int channelNumber);

	/**
	 * @brief		controls a given output according to the provided command.  Possible commands 
	 *				are "Open", "Close", "Toggle", "Open Pulse", and "Close Pulse".  The pulse 
	 *				commands must also give a duration.
	 *
	 * @param	connectionUUID	a string representing a connection
	 * @param	channelNumber	the channel we wish to get the state of
	 *
	 * @return  0 - LOW, 1 - HIGH
	 */
	JMPDLL_API int ControlOutput(const char* connectionUUID, const char* command, int channelNumber, int duration);

	/*
	 * helper functions for output control.  this relieves the need for the user to supply the string,
	 *  which can be prone to typo.  could supply an enum and then convert to a string but
	 *  this seems to be a more straight call.
	 */
	JMPDLL_API int CloseOutput(const char* connectionUUID, int channelNumber);
	JMPDLL_API int OpenOutput(const char* connectionUUID, int channelNumber);
	JMPDLL_API int ToggleOutput(const char* connectionUUID, int channelNumber);
	JMPDLL_API int ClosePulseOutput(const char* connectionUUID, int channelNumber, int duration);
	JMPDLL_API int OpenPulseOutput(const char* connectionUUID, int channelNumber, int duration);



	/*****   REGISTRY   **********************************************************/

	/**
	 * @brief		Reads and array of registry keys
	 *
	 *		Reads and array of registry keys from the connection belonging to the connectionUUID.  
	 *		This function blocks until a response is received and the returned values are 
	 *		exptracted from the message and assigned to the REGISTRY KEY ojects.
	 *
	 * @param		connectionUUID		A string representing a JMP connection
	 * @param		registryKeys		The array of registry keys
	 * @param		keyCount			The number of registry keys in the array
	 *
	 * @return		OK					if successful
	 *				INVLAID_UUID		if the UUID does not exist
	 */
		JMPDLL_API int ReadRegistryKeys(const char* connectionUUID, REGISTRY_KEY* registryKeys, int keyCount);



	/*****   EXTERNAL DEVICES   **************************************************/

	/**
	 * @brief		Used to get a list of device IDs for the connected external modules
	 *
	 * @param		connectionUUID		A string representing a JMP connection
	 * @param		connectedDevices	The array of module ids that will get filled in
	 */
	JMPDLL_API int EnumerateDevices(const char* connectionUUID, char** connectedDevices);

	/**
	 * @ brief		Gets the temperature in Celsius from the module with the given deviceId.  the 
	 *				temperature is assigned to the tempC argument that was passed in by reference
	 */
	JMPDLL_API int GetTemperature(const char* connectionUUID, const char* deviceId, TEMPERATURE* temp_struct);
	JMPDLL_API int GetTemperatureByChannel(const char* connectionUUID, int channel, TEMPERATURE* temp_struct);

	/**
	 * Gets the enironmental properties from the module with the given device id.  the properties are
	 *  assigned to the structure that is passed in by reference.
	 */
	JMPDLL_API int GetEnviron(const char* connectionUUID, const char* deviceId, ENVIRON* environ_struct);

	JMPDLL_API int GetTenVolt(const char* connectionUUID, const char* deviceId, TEN_VOLT* ten_volt_struct);

	JMPDLL_API int SetTenVolt(const char* connectionUUID, const std::string deviceId, const TEN_VOLT* ten_volt_struct);

	/**
	 * @brief		This function sets a channel of a ten volt module to a given percentage.  The 
	 *				module is determined by deviding the given channel number by the number of 
	 *				outputs per module.  Since a TenVolt module has 2 outputs, channel 5 would be 
	 *				the first output on the third module.  The correct module is automatically 
	 *				determined within the code.
	 */
	JMPDLL_API int SetTenVoltChannelPercentage(const char* connectionUUID, int channel, double percentage);
}


#endif
