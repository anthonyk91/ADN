/*
 * PointRep.cpp
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#include "PointRep.h"
#include "Singleton.h"

namespace pointrep {
using namespace utils;

PointRep* PointRep::_brInstance = NULL;

PointRep::PointRep() {
	if (_brInstance != NULL) throw SingletonInstExistsException("PointRep");

	_regInt(_atomSymbols, IKEY_ATOMSYMBOLS);

	_regDbl(_matchThreshold,FKEY_IMMATCHTHRESH);
	_regDbl(_activeBuffer,FKEY_ACTIVEBUFFER);

    _brInstance = this;
}

PointRep::~PointRep() {
	_brInstance = NULL;
	//if (_fragParams) delete _fragParams;
}

PointRep& PointRep::instance() {
	if (_brInstance == NULL) throw SingletonNoInstException("PointRep");
	return *_brInstance;
}

int bin2Int(const vector<FValue>& bitString, int startIdx, int length) {
	int value = 0;
	int power = 1;
	for (int idx = startIdx + length - 1; idx >= startIdx; --idx) {
		if (bitString[idx].iVal) value += power;
		power *= 2;
	}
	return value;
}

}
