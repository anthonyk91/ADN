#ifndef LOGGING_H_
#define LOGGING_H_

#include "logger.h"
#include <sstream>

namespace logger
{

/* super class for objects that do logging, useful for specifying specific
 * write channel for this object.
 * 
 * standard use, subclass public and specify the default channel during construction
 * to write, dump to the string stream (_lss) then call log(..) to dump the
 * given record.
 * 
 * eg:
 * 
 * _lss << "got some number: " << var;
 * log();
 * 
 * calling log() will clear the _lss buffer
 * 
 */
 
class Logging
{
public:
	Logging(int defChannel = LOGGER_GENERAL_ID, log_level currentLevel = TRACE);
	Logging(const Logging& copy);
    virtual ~Logging();
    
    Logging& operator=(const Logging& copy);
    int defChannel();
    log_level curLevel();
    
    mutable ostringstream _lss; // log string stream
    void log(log_level level = L_NOTSET, int channel = LOGGER_NOTSET_ID) const;
    void logn(log_level level = L_NOTSET, int channel = LOGGER_NOTSET_ID) const;
    void doLog(string message, log_level level = L_NOTSET, int channel = LOGGER_NOTSET_ID) const;
    
    bool doOut(log_level level = L_NOTSET, int channel = LOGGER_NOTSET_ID) const;

protected:
    int _defaultChannel;
    log_level _currentLevel;
    
};

}

#endif /*LOGGING_H_*/
