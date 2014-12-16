#include "ParameterParser.h"
#include "CTokenizer.h"
#include "Singleton.h"
#include <fstream>
#include "logger.h"
#include <boost/algorithm/string/trim.hpp>

namespace utils {
using namespace logger;

ParameterParser* ParameterParser::_pinstance = NULL;

ParameterParser::ParameterParser()
{
    if (_pinstance != NULL) throw(std::runtime_error("attempt to create second singleton instance"));

    _pinstance = this;
    _streamRead = "";
}

ParameterParser::~ParameterParser()
{
    _pinstance = NULL;
}

ParameterParser& ParameterParser::getInstance()
{
    if (_pinstance == NULL) SingletonNoInstException("ParameterParser");

    return *_pinstance;
}

bool ParameterParser::instanceAvailable()
    { return (_pinstance != NULL); }

bool ParameterParser::parseFile(const string& filename)
{
    ifstream fileStream;
	fprintf(stderr,"attempt to open file %s\n", filename.c_str());
    fileStream.open(filename.c_str());
    if (fileStream.is_open()) {
    	parseStream(fileStream);
    	return true;
    }
    return false;
}

void ParameterParser::parseStream(istream& instream)
{
    string thisLine;
    vector<string> tokens;
    CIsChar matchPredicate(SINGLE_PRED);
    CIsChar multiPredicate(MULTI_PRED);
    CIsChar metaPredicate(META_PRED);
    CIsChar commentPredicate(PARAMETER_COMMENTS);
    while(instream.good()) {
    	getline(instream, thisLine);
    	_streamRead.append(thisLine);
    	_streamRead.append("\n");
    	
        // separate comments (must begin line with comment symbol)
        //if (StringUtils::stripWhite(thisLine).at(0) == PARAMETER_COMMENTS)
        //    continue;

		// separate comments, remove anything from token
        CTokenizer<CIsChar>::split(tokens, thisLine, commentPredicate);
        thisLine = tokens[0];
        
        // split on first '=' character
        enum matchType {
        	single,
        	multi,
        	other
        };
        matchType thisType = other;
        CTokenizer<CIsChar>::split(tokens, thisLine, matchPredicate);
        if (tokens.size() > 1) thisType = single;
        else { // no split character found, attempt to split on multi val character
	        CTokenizer<CIsChar>::split(tokens, thisLine, multiPredicate);
	        if (tokens.size() > 1) thisType = multi;
	        else { // look for special command	        	
		        CTokenizer<CIsChar>::split(tokens, thisLine, metaPredicate);	        	
	        }
        } 

        // remove leading & trailing whitespace
        string key = boost::trim_copy(tokens[0]);
        string value;
        if (tokens.size() == 2) value = boost::trim_copy(tokens[1]);
        
        // assign pair
        if (! key.empty()) {
        	switch (thisType) {
        	case single:
        		setValue(key, value);
        		break;
        	case multi:
        		addMulti(key, value);
        		break;
        	case other:
        		// check for special command
        		if (! key.compare(IMPORT_CMD)) {
        			// import given file
        			parseFile(value);
        		} else {
        			fprintf(stderr,"unknown command %s\n", thisLine.c_str());        			
        		}
        	}
	    }
    }
}

void ParameterParser::addMulti(const string& key, const string& value)
{
	// for multi variables
	_multiMap[key].push_back(value);
}

void ParameterParser::setValue(const string& key, const string& value)
{
	if (_stringMap.find(key) != _stringMap.end()) {
		logout << "tag " << key << " already recorded" << endl;
		writeBufWarn();
	}
	_stringMap[key] = value; // replace if key exists
}

const string& ParameterParser::grab(const string& key, bool reqd) {
    Maptype::iterator pos;
    if ((pos = _stringMap.find(key)) != _stringMap.end()) {
    	if (reqd && pos->second.empty()) throw MissingValueException(key); 
    	return pos->second;
    }
    else throw NotFoundException(key);
}

double ParameterParser::grabDbl(const string& key)
    { return readDbl(grab(key, true)); }
int ParameterParser::grabInt(const string& key) 
    { return readInt(grab(key, true)); }

/*bool ParameterParser::setIfDef(string& param, const string& key) {
    Maptype::iterator pos;
    if ((pos = _stringMap.find(key)) != _stringMap.end()) {
        param = pos->second;
        return true;
    }
    return false;
}*/

// empty vector if not found
const ParameterParser::Listtype& ParameterParser::getList(const string& key)
{
	return _multiMap[key];
}




bool ParameterParser::setIfDefAlt(string& param, const string& key, const string& def) {
    Maptype::iterator pos;
    if ((pos = _stringMap.find(key)) != _stringMap.end()) {
        param = pos->second;
        return true;
    }
    param = def;
    return false;
}
bool ParameterParser::setIfDefFloatAlt(float& param, const string& key, float def) {
    Maptype::iterator pos;
    if ((pos = _stringMap.find(key)) != _stringMap.end()) {
        param = readDbl(pos->second);
        return true;
    }
    param = def;
    return false;    
}
bool ParameterParser::setIfDefDblAlt(double& param, const string& key, double def) {
    Maptype::iterator pos;
    if ((pos = _stringMap.find(key)) != _stringMap.end()) {
        param = readDbl(pos->second);
        return true;
    }
    param = def;
    return false;
}

bool ParameterParser::setIfDefIntAlt(int& param, const string& key, int def) {
    Maptype::iterator pos;
    if ((pos = _stringMap.find(key)) != _stringMap.end()) {
        param = readInt(pos->second);
        return true;
    }
    param = def;
    return false;    
}

string ParameterParser::getStreamRead() {
	return _streamRead;
}

void ParameterParser::getAllDefs(ostream& os)
{
    Maptype::iterator sIter;
    for(sIter = _stringMap.begin(); sIter != _stringMap.end(); ++sIter) {
    	const string& thisKey = sIter->first;
    	string& thisVal = sIter->second;
    	
    	os << thisKey << " " << SINGLE_PRED << " " << thisVal << endl; 
    }
    
    MultiMaptype::iterator mIter;
    for(mIter = _multiMap.begin(); mIter != _multiMap.end(); ++mIter) {
    	const string& thisKey = mIter->first;
    	Listtype& thisList = mIter->second;
    	
    	os << thisKey << MULTI_PRED << " ";
    	Listtype::iterator lIter;
    	for(lIter = thisList.begin();  lIter != thisList.end(); ++lIter) {
    		string& thisEntry = *lIter;
    		if (lIter != thisList.begin()) os << ", "; 
    		os << thisEntry;
    	}
    	os << endl;
    }
	
}

bool ParameterParser::updateParam(const string& tag, const string& value)
{
	// check each registered var and update
	bool found = false;
	std::tr1::unordered_map<string, list<int*> >::iterator iiter;
	if ((iiter = _intMap.find(tag)) != _intMap.end()) {
		list<int*>::iterator lIter;
		int newVal = atoi(value.c_str());
		_foreach(iiter->second, lIter) {
			**lIter = newVal;
		}
		found = true;
	}
	tr1::unordered_map<string, list<float*> >::iterator fiter;
	if ((fiter = _floatMap.find(tag)) != _floatMap.end()) {
		list<float*>::iterator lIter;
		float newVal = atof(value.c_str());
		_foreach(fiter->second, lIter) {
			**lIter = newVal;
		}
		found = true;
	}
	tr1::unordered_map<string, list<double*> >::iterator diter;
	if ((diter = _dblMap.find(tag)) != _dblMap.end()) {
		list<double*>::iterator lIter;
		double newVal = atof(value.c_str());
		_foreach(diter->second, lIter) {
			**lIter = newVal;
		}
		found = true;
	}
	return found;
}


// registration routines.  when registering, look up existing value
// (and enforce).  on update call, each registered var will be adjusted
bool ParameterParser::registerInt(int* val, const string& tag, int def, bool rq)
{
	bool exists = _intMap.find(tag) != _intMap.end();
	if (rq) {
		// check if tag exists
		*val = grabInt(tag);
	} else {
		_seekInt(*val, tag, def);
	}
	_intMap[tag].push_back(val);
	return exists;
}
bool ParameterParser::registerFlt(float* val, const string& tag, float def, bool rq)
{
	bool exists = _intMap.find(tag) != _intMap.end();
	if (rq) {
		// check if tag exists
		*val = grabDbl(tag); ;
	} else {
		_seekFloat(*val, tag, def);
	}
	_floatMap[tag].push_back(val);
	return exists;
}
bool ParameterParser::registerDbl(double* val, const string& tag, double def, bool rq)
{
	bool exists = _intMap.find(tag) != _intMap.end();
	if (rq) {
		// check if tag exists
		*val = grabDbl(tag);
	} else {
		_seekDbl(*val, tag, def);
	}
	_dblMap[tag].push_back(val);
	return exists;
}


} // namespace utils
