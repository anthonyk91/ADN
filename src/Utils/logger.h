
#ifndef LOGGER_H
#define LOGGER_H
// class for handling logging trace information to stdout or to a file

#include <string>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <boost/thread/mutex.hpp>

#define INITIAL_BUFFER_SIZE 1000
#include "macros.h"

namespace logger {
using namespace std;

class GeneralException : public std::exception
{
    public:
        GeneralException(string msg) : _msg(msg) {}
        virtual ~GeneralException() throw() {}
        
        virtual const char* what() const throw();
        
        string _msg;
};


enum log_level {
    L_NOTSET = 0,
    DEBUG = 1,
    TRACE = 2,
    NOTICE = 3,
    WARNING = 4,
    ERROR = 5,
    CRITICAL = 6,
    H_NOTSET = 7
};

enum channel_flags {
	ACTIVE = 1,
	DATESTAMP = 2
};

void trace(const string& fmt, ...);
void debug(const string& fmt, ...);
void notice(const string& fmt, ...);
void warn(const string& fmt, ...);
void error(const string& fmt, ...);
void critical(const string& fmt, ...);
void lAssert(bool condition, const string& fmt, ...);

void record(int targetID, log_level level, const string& fmt, ...);
void handle_record(int targetID, log_level level, const string& fmt, va_list args);

#define LOGGER_NOTSET_ID -1 
#define LOGGER_GENERAL_ID 1
#define LOGGER_ERROR_ID 2

#define TRACE_OUT 		LOGGER_GENERAL_ID
#define DEBUG_OUT 		LOGGER_GENERAL_ID
#define NOTICE_OUT 		LOGGER_GENERAL_ID
#define WARNING_OUT 	LOGGER_GENERAL_ID
#define ERROR_OUT 		LOGGER_ERROR_ID
#define CRITICAL_OUT 	LOGGER_ERROR_ID

#define logout Logger::getLoggerInstance().getLogBuffer()
#define writeBuf Logger::getLoggerInstance().writeSSBuf

#define writeBufTrace()  Logger::getLoggerInstance().writeSSBuf(TRACE_OUT, TRACE);
#define writeBufDebug()  Logger::getLoggerInstance().writeSSBuf(DEBUG_OUT, DEBUG);
#define writeBufNotice()  Logger::getLoggerInstance().writeSSBuf(NOTICE_OUT, NOTICE);
#define writeBufWarn()  Logger::getLoggerInstance().writeSSBuf(WARNING_OUT, WARNING);
#define writeBufError()  Logger::getLoggerInstance().writeSSBuf(ERROR_OUT, ERROR);
#define writeBufCritical()  Logger::getLoggerInstance().writeSSBuf(CRITICAL_OUT, CRITICAL);

class LogRecorder {
public:
    LogRecorder(ostream* os = NULL, int channel = LOGGER_GENERAL_ID, log_level lowrng = TRACE, log_level highrng = H_NOTSET, bool enbl = true);
    LogRecorder(ostream* os, vector<int> channels, log_level lowrng = TRACE, log_level highrng = H_NOTSET, bool enbl = true);

	virtual ~LogRecorder();
    virtual ostream& stream();
    ostream* _output;
    pair<log_level, log_level> _recordRange;
    bool _enabled;
    
   	void flush();
	bool handlesChannel(int channel);
	void setChannels(vector<int> channels);
	void addChannels(vector<int> channels);
	
private:
    vector<int> _channels;
  	bool _deleteStream;
};

class FileRecorder : public LogRecorder {
public:
	FileRecorder(const string& ofname, int channel = LOGGER_GENERAL_ID, log_level lowrng = L_NOTSET, log_level highrng = H_NOTSET, bool enbl = true);
    FileRecorder(const string& ofname, vector<int> channels, log_level lowrng = L_NOTSET, log_level highrng = H_NOTSET, bool enbl = true);

    virtual ostream& stream();

protected:
    ofstream _fstream;
    string _fileName;
};
class Logger
{
	public:
		Logger();
		virtual ~Logger();
		
        static Logger& getLoggerInstance();
        static void release(); // needs to be called to destroy singleton

        static void setScreenLevel(log_level level);
        static void setThrowLevel(log_level level);

        static void addRecorder(LogRecorder& rec); // must be owned/maintained externally
		const vector<LogRecorder*> getRecorders();
		void removeRecorder(LogRecorder& rec);

        static ostringstream& getLogBuffer();
		void writeSSBuf(int targetID = LOGGER_GENERAL_ID, log_level level = TRACE);
		
		bool acceptOutput(int targetID, log_level level);
		void record(int targetID, log_level level, const string& msgString);

		void setChannel(int channel, unsigned int flag, bool value = true);
        bool channelSet(int channel, unsigned int flag);
		void setChannelScreenLevel(int channel, log_level screenLevel);

		static void Flush();
		void flush();

		char* getBuffer(unsigned int size);
		unsigned int currentBufferSize();

	protected:
		// declare default constructors protected so local instantiations can't be made
		Logger(const Logger&);
        Logger& operator= (const Logger&);
        
        log_level _screen_level; // any messages at or below this level are disregarded
        log_level _throw_level;  // any messages at or above this level throw an exception
        log_level _fmt_level;    // " above this level are preceded with an identifier (eg "WRN: ")
        
        vector<LogRecorder*> _recorders;
        map<int, unsigned int> _channelMap; // flags for each channel
        map<int, log_level> _channelScreenLevel; // ignore level for each channel
		
        ostringstream _logSSBuf;
        boost::mutex _mutex;
        
  	private:
        static Logger* pinstance;
	    char *_msgBuffer;
    	unsigned int _bufferSize;
};

};

#endif // LOGGER_H
