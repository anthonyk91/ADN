#ifndef PARAMETERPARSER_H_
#define PARAMETERPARSER_H_

#define PARAMETER_COMMENTS '#'
#include <stdexcept>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <stdlib.h>

#include <tr1/unordered_map>

namespace utils {
using namespace std;
	
#define _regInt(var, label) utils::ParameterParser::_pinstance->registerInt((&var), (label))
#define _regFlt(var, label) utils::ParameterParser::_pinstance->registerFlt((&var), (label))
#define _regDbl(var, label) utils::ParameterParser::_pinstance->registerDbl((&var), (label))

#define _regIntOp(var, label, def) utils::ParameterParser::_pinstance->registerInt((&var), (label), (def), false)
#define _regFltOp(var, label, def) utils::ParameterParser::_pinstance->registerFlt((&var), (label), (def), false)
#define _regDblOp(var, label, def) utils::ParameterParser::_pinstance->registerDbl((&var), (label), (def), false)

#define _regIntCond(var, label, def, cond) utils::ParameterParser::_pinstance->registerInt((&var), (label), (def), (cond))
#define _regFltCond(var, label, def, cond) utils::ParameterParser::_pinstance->registerFlt((&var), (label), (def), (cond))
#define _regDblCond(var, label, def, cond) utils::ParameterParser::_pinstance->registerDbl((&var), (label), (def), (cond))

#define _findInt(label) utils::ParameterParser::_pinstance->grabInt(label)
#define _findFloat(label) utils::ParameterParser::_pinstance->grabDbl(label)
#define _findDbl(label) utils::ParameterParser::_pinstance->grabDbl(label)
#define _findStr(label) utils::ParameterParser::_pinstance->grab(label, true)

#define _seekInt(a,b,c) utils::ParameterParser::_pinstance->setIfDefIntAlt((a),(b),(c))
#define _seekFloat(a,b,c) utils::ParameterParser::_pinstance->setIfDefFloatAlt((a),(b),(c))
#define _seekDbl(a,b,c) utils::ParameterParser::_pinstance->setIfDefDblAlt((a),(b),(c))
#define _seekStr(a,b,c) utils::ParameterParser::_pinstance->setIfDefAlt((a),(b),(c))
#define _seekDef(b) utils::ParameterParser::_pinstance->keyDefined(b)

#define IMPORT_CMD "import"
#define SINGLE_PRED '='
#define MULTI_PRED ':'
#define META_PRED ' '

class ParameterParserException : public std::runtime_error {
	public:
		ParameterParserException(const string& arg) : runtime_error(arg) {}
};
class NotFoundException : public ParameterParserException
{
    public:
        NotFoundException(const string& arg) : ParameterParserException("Not Found Exception: " + arg) {}
        NotFoundException() : ParameterParserException("Not Found Exception") {}            
};
class MissingValueException : public ParameterParserException
{
    public:
    	MissingValueException(const string& arg) : ParameterParserException("Missing Value Exception: " + arg) {}
    	MissingValueException() : ParameterParserException("Missing Value Exception") {}            
};

class ParameterParser
{
public:
	ParameterParser();
	virtual ~ParameterParser();
    
	typedef map<string, string> Maptype;
	typedef list<string> Listtype;
	typedef map<string, Listtype > MultiMaptype;

    bool parseFile(const string& filename);
    void parseStream(istream& instream);
	void addMulti(const string& tag, const string& value); // for multi variables
	void setValue(const string& tag, const string& value);

    static ParameterParser& getInstance();
    static bool instanceAvailable();

	bool keyDefined(const string& key);
	
    // assign value of key to given variable if set else assign default value
    bool setIfDefAlt(string& param, const string& key, const string& defValue);
    bool setIfDefFloatAlt(float& param, const string& key, float defValue);
    bool setIfDefDblAlt(double& param, const string& key, double defValue);
    bool setIfDefIntAlt(int& param, const string& key, int defValue);

    const string& grab(const string& key, bool reqd = false);
    double grabDbl(const string& key);
    int grabInt(const string& key);

	// multi-map settings
	const Listtype& getList(const string& key); // empty vector if not found

	string getStreamRead();
	void getAllDefs(ostream& os);
	
	bool updateParam(const string& tag, const string& value);

	bool registerInt(int* val, const string& tag, int def = 0, bool rq = true);
	bool registerFlt(float* val, const string& tag, float def = 0.0, bool rq = true);
	bool registerDbl(double* val, const string& tag, double def = 0.0, bool rq = true);

    static ParameterParser* _pinstance;

protected:
    Maptype _stringMap;
    MultiMaptype _multiMap;
    
    inline double readDbl(const string& str);
    inline int readInt(const string& str);
    

	string _streamRead; // store stream read 

	tr1::unordered_map<string, list<int*> > _intMap;
	tr1::unordered_map<string, list<float*> > _floatMap;
	tr1::unordered_map<string, list<double*> > _dblMap;
};

inline double ParameterParser::readDbl(const string& str)
    { return atof(str.c_str()); }

inline int ParameterParser::readInt(const string& str) 
    { return atoi(str.c_str()); }

inline bool ParameterParser::keyDefined(const string& key)
{ 
    return (_stringMap.find(key) != _stringMap.end());
}		

} // namespace utils

#endif /*PARAMETERPARSER_H_*/
