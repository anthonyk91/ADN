#include "Logging.h"

namespace logger
{

Logging::Logging(int defChannel, log_level currentLevel)
    : _defaultChannel(defChannel), _currentLevel(currentLevel)
{

}

Logging::Logging(const Logging& copy)
    : _defaultChannel(copy._defaultChannel), _currentLevel(copy._currentLevel)
{
    
}

Logging::~Logging()
{
}

Logging& Logging::operator=(const Logging& copy)
{
	if (&copy == this) {
		warn("attempted operator= on self (Logging)\n");
		return *this;
	}
	_defaultChannel = copy._defaultChannel;
	_currentLevel = copy._currentLevel;
}

void Logging::logn(log_level level, int channel) const
{
	_lss << endl;
	log(level, channel);
}

void Logging::log(log_level level, int channel) const
{
    doLog(_lss.str(), level, channel);
    _lss.str("");
}

int Logging::defChannel() { return _defaultChannel; }
log_level Logging::curLevel() { return _currentLevel; }

void Logging::doLog(string message, log_level level, int channel) const
{
    static Logger& sLogger = Logger::getLoggerInstance();

    if (level == L_NOTSET) level = _currentLevel;
    if (channel == LOGGER_NOTSET_ID) channel = _defaultChannel;
    
    sLogger.record(channel, level, message);   
}

bool Logging::doOut(log_level level, int channel) const
{
    static Logger& sLogger = Logger::getLoggerInstance();

    if (level == L_NOTSET) level = _currentLevel;
    if (channel == LOGGER_NOTSET_ID) channel = _defaultChannel;
    
    return sLogger.acceptOutput(channel, level);
}


}
