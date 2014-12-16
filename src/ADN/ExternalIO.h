#ifndef EXTERNALIO_H_
#define EXTERNALIO_H_

#include "logger.h"
#include <iostream>
#include <sstream>
#include "boost/filesystem/operations.hpp"
#include "ParameterParser.h"
#include "Logging.h"

#define ICMDKEY_SCRIPTFILE "script-file,s"
#define ICMDKEY_SCRIPTFILE_DESC "script file"

#define ICMDKEY_SCRIPTSTDIN "script-stdin,i"
#define ICMDKEY_SCRIPTSTDIN_DESC "read script from stdin"

#define ICMDKEY_SEEDVALUE "seed-value,n"
#define ICMDKEY_SEEDVALUE_DESC "random number seed"

#define ICMDKEY_VERSION "version,v"
#define ICMDKEY_VERSION_DESC "show version"

#define SKEY_RUNNAME "runName"
#define SKEY_RESULTPATH "resultPath"

#define MKEY_OUTPUTLIST "setOutput"
#define MSUBKEY_OUTPUTLIST_STREAMTYPE "stream"
#define MSUBKEY_OUTPUTLIST_FILETYPE "file"
#define MSUBKEY_OUTPUTLIST_STDOUT "stdout"
#define MSUBKEY_OUTPUTLIST_STDERR "stderr"
#define SKEY_DATESTAMPLIST "datestamp"
#define MKEY_SCREENLEVELLIST "channelScreenLevel"


namespace adn {
using namespace logger;
using namespace utils;

class ExternalIO : public Logging
{
public:
	ExternalIO();
	virtual ~ExternalIO();

	bool readCommandLine(int argc, char* argv[]);
	
protected:
	void initialiseOutput(bool chpath = true);
	Logger _loggerObject;  //< for handling output to stdout or file logs
	LogRecorder* _stdoutRec;
	LogRecorder* _stderrRec;
	
	ParameterParser _parameterData;  //< for recording data parameters from script
	bool _showpid;
};

}

#endif /*EXTERNALIO_H_*/
