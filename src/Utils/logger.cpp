#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <time.h>
#include "Singleton.h"

namespace logger {
using namespace utils;

static const char* Log_Labels[] = {
    "",
    "TRC",
    "DBG",
    "NTC",
    "WRN",
    "ERR",
    "CRT",
    ""
};

const char* GeneralException::what() const throw() { return _msg.c_str(); }

#define _Create_ap   va_list ap; va_start(ap, fmt);
void debug(const string& fmt, ...)   { _Create_ap; handle_record(DEBUG_OUT, DEBUG, fmt, ap); va_end(ap);}
void trace(const string& fmt, ...)   { _Create_ap; handle_record(TRACE_OUT, TRACE, fmt, ap); va_end(ap);}
void notice(const string& fmt, ...)  { _Create_ap; handle_record(NOTICE_OUT, NOTICE, fmt, ap); va_end(ap);}
void warn(const string& fmt, ...)    { _Create_ap; handle_record(WARNING_OUT, WARNING, fmt, ap); va_end(ap);}
void error(const string& fmt, ...)   { _Create_ap; handle_record(ERROR_OUT, ERROR, fmt, ap); va_end(ap);}
void critical(const string& fmt, ...){ _Create_ap; handle_record(CRITICAL_OUT, CRITICAL, fmt, ap); va_end(ap);}
void lAssert(bool condition, const string& fmt, ...) { _Create_ap; if(! condition) handle_record(LOGGER_ERROR_ID, ERROR, fmt, ap); va_end(ap);}

void record(int targetID, log_level level, const string& fmt, ...){ _Create_ap; handle_record(targetID, level, fmt, ap); }
void handle_record(int targetID, log_level level, const string& fmt, va_list args)
{
    Logger& logInstance = Logger::getLoggerInstance();

    // decompose argument list into a single formatted string
	unsigned int size = logInstance.currentBufferSize();
	char* msgBuffer;

	bool writtenFully = false;
	while (! writtenFully) {
		int result;
		msgBuffer = logInstance.getBuffer(size);

		#ifdef __GNUC__
			result = vsnprintf(msgBuffer, size+1 /*include \0 */, fmt.c_str(), args);
		#else
			result = _vsnprintf(msgBuffer, size+1, fmt.c_str(), args);
		#endif
		if (result == -1 ) size *= 2;			// double buffer
		else if (result > (int) size) size = _max((int) size * 2, result); 	// given required size, resize to that
		else writtenFully = true;				// successfully written to buffer
	}

    logInstance.record(targetID, level, msgBuffer);
}

/* static member functions */

void Logger::setScreenLevel(log_level level) {
    // this is a member function, even if its static, so we should be able to set member variables
    // without violating encapsulation.
    Logger::getLoggerInstance()._screen_level = level;
}

void Logger::setThrowLevel(log_level level) {
    Logger::getLoggerInstance()._throw_level = level;
}

Logger* Logger::pinstance = NULL; // initialise singleton pointer to 0
Logger& Logger::getLoggerInstance()
{
    if(pinstance == NULL) throw SingletonNoInstException("Logger");

	return (*pinstance);
}


/* object functions */
Logger::Logger()
{
    if(pinstance != NULL) throw SingletonInstExistsException("Logger");

	//ctor
	// set default levels
	_screen_level = L_NOTSET;
	_throw_level = ERROR;
	_fmt_level = WARNING;

    // assign recorders for stderr and stdout
    //_recorders.push_back(&_stdoutRec);
    //_recorders.push_back(&_stderrRec);
    _bufferSize = INITIAL_BUFFER_SIZE;
    _msgBuffer = new char[_bufferSize + 1];

    pinstance = this;
}

Logger::~Logger() {
	//dtor
	pinstance = NULL;

	// delete recorders
	vector<LogRecorder*>::iterator iter;
	_foreach(_recorders, iter) delete *iter;

	if (_msgBuffer != NULL) delete[] _msgBuffer;
}

void Logger::addRecorder(LogRecorder& rec) {
    Logger::getLoggerInstance()._recorders.push_back(&rec);
}

const vector<LogRecorder*> Logger::getRecorders() {
    return Logger::getLoggerInstance()._recorders;
}

void Logger::removeRecorder(LogRecorder& rec) {
	vector<LogRecorder*>::iterator match = find(_recorders.begin(), _recorders.end(), &rec);
    if (match != _recorders.end()) Logger::getLoggerInstance()._recorders.erase(match);
}

void Logger::Flush() { Logger::getLoggerInstance().flush(); }
void Logger::flush()
{
    vector<LogRecorder*>::iterator iter;
    _foreach(_recorders, iter) {
        LogRecorder& rec = **iter;
        if (rec._enabled) rec.flush();
    }
}

bool Logger::acceptOutput(int targetID, log_level level)
{
	if (! channelSet(targetID, ACTIVE)) return false;

    log_level screenLevel = _screen_level;

	// check if specific ignore level for channel
    map<int, log_level>::iterator f = _channelScreenLevel.find(targetID);
    if (f != _channelScreenLevel.end()) screenLevel = f->second;

	return level > screenLevel;
}

ostringstream& Logger::getLogBuffer()
{
	return getLoggerInstance()._logSSBuf;
}

void Logger::writeSSBuf(int targetID, log_level level)
{
	boost::mutex::scoped_lock(_mutex);
	record(targetID, level, _logSSBuf.str());
	_logSSBuf.str("");
}

#define _InRange(lev, rg) (((lev) >= (rg).first) && ((lev) <= (rg).second))
void Logger::record(int targetID, log_level level, const string& msgString) {
    // dump message to appropriate location(s)
    using namespace std;

    if (! acceptOutput(targetID, level)) return;

    ostringstream oss;
    istringstream iss(msgString);
    string line;
    while (std::getline(iss, line)) {
		if (level >= _fmt_level) {
			oss << Log_Labels[level] << ": ";
		}
		if (channelSet(targetID, DATESTAMP)) {
			static time_t curtime;
			time(&curtime);
			struct tm& ct = *localtime(&curtime);
			char dateBuf[1000];
			sprintf(dateBuf, "%d/%d/%4d %d:%02d:%02d ", ct.tm_mday,
				ct.tm_mon + 1, (ct.tm_year + 1900), ct.tm_hour,
				ct.tm_min, ct.tm_sec);
			oss << dateBuf;
		}

		oss << line << endl;
    }

    vector<LogRecorder*>::iterator iter;
    string msg = oss.str();
    _foreach(_recorders, iter) {
        LogRecorder& rec = **iter;
        if (rec._enabled && rec.handlesChannel(targetID) && _InRange(level, rec._recordRange)) {
            rec.stream() << msg;
            rec.flush();
        }
    }

    // throw exception if set
    if (level >= _throw_level) {
    	// clear buffers
        _foreach(_recorders, iter) {
            LogRecorder& rec = **iter;
            if (rec._enabled && rec.handlesChannel(targetID) && _InRange(level, rec._recordRange))
                rec.stream().flush();
        }
    	throw GeneralException(msgString);
    }
}

void Logger::setChannel(int channel, unsigned int flag, bool value)
{
	if (value) _channelMap[channel] |= flag;
	else _channelMap[channel] &= ~flag;
}

bool Logger::channelSet(int channel, unsigned int flag)
{
	map<int,unsigned int>::iterator pos;
	if ((pos = _channelMap.find(channel)) != _channelMap.end()) {
		return (*pos).second & flag;
	}
	return true; // if not set, let it be active
}

void Logger::setChannelScreenLevel(int channel, log_level level)
{
	_channelScreenLevel[channel] = level;
}

char* Logger::getBuffer(unsigned int size)
{
	if (size > _bufferSize) {
		_bufferSize = size;
		if (_msgBuffer != NULL) delete[] _msgBuffer;
		_msgBuffer = new char[_bufferSize + 1];
	}

	return _msgBuffer;
}

unsigned int Logger::currentBufferSize()
	{ return _bufferSize; }

// dependencies:
//  getLoggetInstance() does not return null (make it throw if can't create)

LogRecorder::LogRecorder(ostream* os, int channel, log_level lowrng, log_level highrng, bool enbl)
    : _output(os), _recordRange(lowrng, highrng), _enabled(enbl)
{
	_channels.push_back(channel);
}

LogRecorder::LogRecorder(ostream* os, vector<int> channels, log_level lowrng, log_level highrng, bool enbl)
    : _output(os), _recordRange(lowrng, highrng), _enabled(enbl),
    _channels(channels) {}

LogRecorder::~LogRecorder()
{
}

void LogRecorder::flush()
{
	if (_output) _output->flush();
}

ostream& LogRecorder::stream() {
	// request for stream object
	return *_output;
}

void LogRecorder::setChannels(vector<int> channels)
{
	_channels = channels;
}

void LogRecorder::addChannels(vector<int> channels)
{
	_channels.insert(_channels.end(), channels.begin(), channels.end());
}

bool LogRecorder::handlesChannel(int channel)
{
	return find(_channels.begin(), _channels.end(), channel) != _channels.end();
}

FileRecorder::FileRecorder(const string& ofname, int channel, log_level lowrng, log_level highrng, bool enbl)
	: LogRecorder(NULL, channel, lowrng, highrng, enbl), _fileName(ofname)
{
	_fstream.open(ofname.c_str());
	_output = &_fstream;
}

FileRecorder::FileRecorder(const string& ofname, vector<int> channels, log_level lowrng, log_level highrng, bool enbl)
	: LogRecorder(NULL, channels, lowrng, highrng, enbl), _fileName(ofname)
{
	_fstream.open(ofname.c_str());
	_output = &_fstream;
}

ostream& FileRecorder::stream() {
	// request for stream object.  check file is open, re-open if necessary
	if (_fstream.fail()) {
		// some problem with output.  close and re-open (?)
		_fstream.close();
	}
	if (! _fstream.is_open()) {
		// open to append
		_fstream.open(_fileName.c_str(), ios_base::out | ios_base::app);
	}
	return LogRecorder::stream();
}


} // namespace logger

