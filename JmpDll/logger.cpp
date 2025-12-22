#include "logger.hpp"


#include <algorithm> // Required for std::transform
#include <cctype> // Required for std::tolower
#include <string>


std::map<std::string, std::ofstream*> Logger::STREAMS_CACHE_BY_NAME = std::map<std::string, std::ofstream*>();
std::mutex Logger::mtx;


// creates a new instance and opens the log for appending.  a line with dashes 
//  is immediately logged indicating that a new instance of the application 
//  has been tarted.
Logger::Logger(const std::string& filename)
{
	// check to see if the same name is being used as a logger we have already instantiated.  use 
	//  a mutex to protect our cache
	mtx.lock();
	bool newInstance = false;

	// get the streams cache
	std::map<std::string, std::ofstream*> streamsCacheByName = getMap();

	std::string lowerFilename = filename;
	std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
		[](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
	if (streamsCacheByName.end() == streamsCacheByName.find(lowerFilename)) {
		streamsCacheByName.insert_or_assign(filename, new std::ofstream());
		newInstance = true;
	}
	mtx.unlock();

	logFile = streamsCacheByName[filename];
	logFile->open(filename, std::ios::app);
	if (!logFile->is_open()) {
		std::cerr << "Error opening log file." << std::endl;
	}

	if (newInstance) {
		this->log("--------------------");
	}
}



// cleanup
Logger::~Logger() {
	logFile->flush();
	logFile->close();
	delete logFile;
}


// a warning, so prepend a *
void Logger::warn(std::string message) {
	log(std::string(" * ") + message);
}



// an error, so prepend a **
void Logger::error(std::string message) {
	log(std::string("** ") + message);
}



// perform the logging
void Logger::log(std::string message)
{
	mtx.lock();

	//// Get current timestamp
	//time_t now = time(0);
	//struct tm timeinfo;
	//localtime_s(&timeinfo, &now);
	//char timestamp[20];
	//strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S.%f", &timeinfo);

	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	char buf[64];
	std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S", &tm);
	char timestamp[80];
#ifdef _MSC_VER
	sprintf_s(timestamp, sizeof(timestamp), "%s.%03d", buf, static_cast<int>(ms.count()));
#else
	std::snprintf(timestamp, sizeof(timestamp), "%s.%03d", buf, static_cast<int>(ms.count()));
#endif
	std::puts(timestamp);



	// Create log entry
	std::ostringstream logEntry;
	logEntry << "[" << timestamp << "] " << message << std::endl;

	// Output to console
	std::cout << logEntry.str();

	// Output to log file
	if (logFile->is_open()) {
		*logFile << logEntry.str();
		logFile->flush();
	}

	mtx.unlock();
}
