#pragma once
#ifndef LOGGER_H
#define LOGGER_H


// a vanilla logger.  no library

#include "stdafx.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <mutex>


class Logger {

private:
	static std::map<std::string, std::ofstream*> STREAMS_CACHE_BY_NAME;
	static std::mutex mtx;
	const std::map<std::string, std::ofstream*>& getMap() {
		static const auto* map = new std::map<std::string, std::ofstream*>{ };
		return *map;
	}

public:
	/**
	 * @brief		Constructor to initialize a logger
	 *
	 * @param		filename		The filename to use for the logger
	 */
	Logger(const std::string& filename);

	/**
	 * @brief		Deconstructor to cleanup up the logger
	 */
	~Logger();

	/**
	 * @brief		Logs a message to the log file.  The message is prepended with one ASTRISK
	 *
	 * @param		message			The message that should be logged
	 */
	void warn(std::string message);

	/**
	 * @brief		Logs a message to the log file.  The message is prepended with two ASTRISKS
	 *
	 * @param		message			The message that should be logged
	 */
	void error(std::string message);

	/**
	 * @brief		Logs a message to the log file.  The message is prepended with a timestamp
	 *
	 * @param		message			The message that should be logged
	 */
	void log(std::string message);

private:	
	std::ofstream* logFile;
	
};


#endif // LOGGER_H