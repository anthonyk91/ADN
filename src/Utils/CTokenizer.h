#ifndef CTOKENIZER_H_
#define CTOKENIZER_H_

// written by George Anescu, provided at http://www.codeproject.com/cpp/stringtok.asp

#include <functional>
#include <string>
#include <vector>
#include <locale>
#include <algorithm>

// predicates

namespace utils {
using namespace std;

class CIsSpace : public unary_function<char, bool>
{
public:
  bool operator()(char c) const;
};

class CIsFromString : public unary_function<char, bool>
{
public:
  //Constructor specifying the separators
  CIsFromString(string const& rostr) : m_ostr(rostr) {}
  bool operator()(char c) const;

private:
  string m_ostr;
};

class CIsChar : public unary_function<char, bool>
{
public:
    CIsChar(char matchChar) : _matchChar(matchChar) {}
    bool operator()(char c) const;

private:
    char _matchChar;
};




template <class Pred=CIsSpace>
class CTokenizer
{
public:
  //The predicate should evaluate to true when applied to a separator.
  static void tokenize(vector<string>& roResult, string const& rostr,
                       Pred const& roPred=Pred());

  // only splits on first token
  static void split(vector<string>& roResult, string const& rostr,
                       Pred const& roPred=Pred());

};

//The predicate should evaluate to true when applied to a separator.
template <class Pred>
inline void CTokenizer<Pred>::tokenize(vector<string>& roResult,
                                            string const& rostr, Pred const& roPred)
{
  //First clear the results vector
  roResult.clear();
  string::const_iterator it = rostr.begin();
  string::const_iterator itTokenEnd = rostr.begin();
  while(it != rostr.end())
  {
    //Eat seperators
    while(it != rostr.end() && roPred(*it)) it++;
    //Find next token
    itTokenEnd = find_if(it, rostr.end(), roPred);
    //Append token to result
    if(it < itTokenEnd)
      roResult.push_back(string(it, itTokenEnd));
    it = itTokenEnd;
  }
}

template <class Pred>
inline void CTokenizer<Pred>::split(vector<string>& roResult,
                                            string const& rostr, Pred const& roPred)
{
  //First clear the results vector
  roResult.clear();
  string::const_iterator itTokenEnd = rostr.begin();

  //Find split point
  itTokenEnd = find_if(rostr.begin(), rostr.end(), roPred);

  //split string to build result
  roResult.push_back(string(rostr.begin(), itTokenEnd));

  // eat separator
  if(itTokenEnd != rostr.end()) {
	  if(roPred(*itTokenEnd)) itTokenEnd++;
      roResult.push_back(string(itTokenEnd, rostr.end()));
  }
}

#include <ctype.h>

inline bool CIsSpace::operator()(char c) const
{
  // isspace<char> returns true if c is a white-space character
  // (0x09-0x0D or 0x20)
  return isspace(c);
}

inline bool CIsFromString::operator()(char c) const
{
  //unsigned int iFind = m_ostr.find(c);
  if(m_ostr.find(c) != string::npos)
    return true;
  else
    return false;
}

inline bool CIsChar::operator()(char c) const
{
  return (_matchChar == c);
}

} // namespace utils

#endif /*CTOKENIZER_H_*/

