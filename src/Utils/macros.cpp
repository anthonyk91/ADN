
#include "macros.h"

namespace utils {

int _getAndIncrement(int& val) {
	int retVal = val;
	++val;
	if (val == INT_MAX) val = 1;
	return retVal;
}

int findEncodingBits(int number) {
	// determine by comparing powers of two
	int x=0;
	while (intexp(2,x) < number) ++x;
	return x;
}

int intexp(int number, int power) {
	int val=1;
	for (int i=0; i<power; ++i) val *= number;
	return val;
}



} // namespace utils
