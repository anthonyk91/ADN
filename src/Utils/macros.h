#ifndef _MACROS_H_
#define _MACROS_H_

#include "logger.h"
#include <stdexcept>
#include <utility>

#include "RNGHandler.h"
#include <limits.h>

namespace utils {
	
#define _BinaryIdentifier "$Name$"

#define _min(a,b) (((a) < (b)) ? (a) : (b))
#define _max(a,b) (((a) > (b)) ? (a) : (b))
#define _trim(a,b,c) _min((_max((a),(b))),(c))
#define _chooseYN(a) ((a) ? "Y" : "N")

#define _inRange(a,b,c) ((a) >= (b) && (a) < (c))

const float pi = 3.1415926536;

// Some common define routines

#define _ClockDiff(a,b) (((double) ((a) - (b))) / CLOCKS_PER_SEC)
#define _ClockTime(a) (((double) (a)) / CLOCKS_PER_SEC)

#define _findInVec(vec, val) (std::find((vec).begin(), (vec).end(), (val)) != (vec).end())

// catch throws
/*#ifdef _DEBUG
#define throw(n) _throw(n)
inline void _throw(std::exception& e) {
	throw(e);
}
#endif*/

// random numbers
#ifndef _NRand
#define _NRand() rand()
#endif

#ifndef _NRandMax
#define _NRandMax RAND_MAX
#endif

#ifndef _fRand
#define _fRand() (_NRand() / (float) _NRandMax) // random float (0..1, trying to avoid 1.0)
#endif

#ifndef _RandOf
#define _RandOf(n) (_NRand() % (n))
#endif

#ifndef _SeedRand
#define _SeedRand(n) (srand(n))
#endif

#ifndef _pRand
#define _pRand(p) (_NRand() < (_NRandMax * p))
#endif

#ifndef _iRand
#define _iRand(p, pmax) (p < (_NRand() % pmax))
#endif

#ifdef _DEBUG
#define STOP_ON_ASSERTS
#endif

#ifndef _pmod
#define _pmod(x,y) ((x % y) < 0 ? x % y + y : x % y)
#endif

#ifdef STOP_ON_ASSERTS
#define _Assert(condition, parameters) if (! (condition)) { logger::debug("Assert failed, file " __FILE__ ", line %d\n", __LINE__); logger::error parameters; }
#define _pAssert(condition) if (! (condition)) { logger::error("Assert failed, file " __FILE__ ", line %d\n", __LINE__); }
#define _sAssert(condition, parameters) if (! (condition)) { logger::debug("Assert failed, file " __FILE__ ", line %d\n", __LINE__); ostringstream _AstStrm; _AstStrm << parameters; logger::error(_AstStrm.str()); }
#define _failAssert logger::error("Assert failed, file " __FILE__ ", line %d\n", __LINE__);
#else
#define _Assert(condition, parameters) if (! (condition)) { logger::warn parameters; }
#define _pAssert(condition) if (! (condition)) { logger::warn("Assert failed\n"); }
#define _sAssert(condition, parameters) if (! (condition)) { ostringstream _AstStrm; _AstStrm << parameters; logger::warn(_AstStrm.str()); }
#define _failAssert logger::warn("Assert failed\n");
#endif

// specific exceptions
class NotImplementedException : public std::runtime_error {
	public:
		NotImplementedException(const std::string& msg) 
            : runtime_error(std::string("Method not implemented: ").append(msg)) {}
};



// handy macros (assumes iter is already defined for the correct type)
#define _foreach(sequence, iter) for((iter) = (sequence).begin(); (iter) != (sequence).end(); (iter)++)
#define _foreachp(sequence, iter) for((iter) = (sequence)->begin(); (iter) != (sequence)->end(); (iter)++)

// generic functions
template <class InputIterator>
int countElements(InputIterator first, InputIterator last);

//template <class InputIterator>
//InputIterator selectRandom(InputIterator first, InputIterator last);

template<class someclass> 
class ptrue  {
	public:
	bool operator()(someclass c) const;
};

template <class InputIterator>
InputIterator selectRandom(InputIterator first, InputIterator last);

template <class InputIterator, class Predicate>
InputIterator selectRandom(InputIterator first, InputIterator last,
	Predicate test);

template <class Collection>
Collection& eraseRandom(Collection& collect);

template <class Collection1, class Collection2>
Collection1& joinVec(Collection1& receiver, const Collection2& adder);

int _getAndIncrement(int& val);

int findEncodingBits(int number);
int intexp(int number, int power);


} // namespace utils

#include "macros.tem"

#endif //_MACROS_H_
