/*
 * ParityplexWorld.cpp
 *
 *  Created on: Nov 4, 2010
 *      Author: anthony
 */

#include "../PointRep.h"

namespace pointrep {


ParityplexWorld::ParityplexWorld()
	: PointWorld()
{
	_regInt(_mplxSize, IKEY_MPLXSIZE); //atoi(parameters[sizeParam].c_str());
	_regInt(_paritySize, IKEY_PARITYSIZE); //atoi(parameters[sizeParam].c_str());
	_binaryClass = true;
	updateBits();
	_epochSize = 10000;


}

ParityplexWorld::~ParityplexWorld() {
	// TODO Auto-generated destructor stub
}

int ParityplexWorld::remainingInsts(int set) const {
	return _epochSize - _trainIdx;
}

void ParityplexWorld::start(int set) {
	_trainIdx = 0;
}

void ParityplexWorld::updateBits() {
	switch(_mplxSize) {
	case 3:
		_dataBits = 2;
		_addressBits = 1;
		break;
	case 6:
		_dataBits = 4;
		_addressBits = 2;
		break;
	case 11:
		_dataBits = 8;
		_addressBits = 3;
		break;
	case 20:
		_dataBits = 16;
		_addressBits = 4;
		break;
	case 37:
		_dataBits = 32;
		_addressBits = 5;
		break;
	case 70:
		_dataBits = 64;
		_addressBits = 6;
		break;
	case 135:
		_dataBits = 128;
		_addressBits = 7;
		break;
	case 264:
		_dataBits = 256;
		_addressBits = 8;
		break;
	}

	// specify bit string lengths
	int size = _mplxSize * _paritySize;

	// specify types
	vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	vector<int>& nClasses = PointRep::_brInstance->_featureClasses;
	vector<pair<FValue,FValue> >& fRanges = PointRep::_brInstance->_featureRanges;

	fTypes.resize(size);
	nClasses.resize(size);
	fRanges.resize(size);
	for (int i=0; i<size; ++i) {
		fTypes[i] = iClass;
		nClasses[i] = 2;
		fRanges[i].first.iVal = 0;
		fRanges[i].second.iVal = 1;
	}
}


//void ParityplexWorld::genBitString() const {} // already encoded

int ParityplexWorld::evaluateAns(PtInstInfo& t) const { return evaluateAns(t._features); }
int ParityplexWorld::evaluateAns(const vector<FValue>& bitString) const {
	// evaluate parity blocks
	_Assert(bitString.size() % _paritySize == 0, ("invalid- bit string (%d) is not a multiple of parity bits (%d)", bitString.size(), _paritySize));
	vector<FValue> mplxBits(_addressBits + _dataBits);

	int idx = 0, mplxIdx = 0;
	int thisParity, j;
	while (idx < bitString.size()) {
		thisParity = 0;
		for (j=0; j < _paritySize; ++j) {
			thisParity ^= bitString[idx++].iVal;
		}
		mplxBits[mplxIdx++].iVal = thisParity;
	}

	// find index from address string
	idx = bin2Int(mplxBits, 0, _addressBits);

	// look up index
	unsigned int findIdx = _addressBits + idx;
	_Assert(findIdx <= mplxBits.size(), ("invalid mplx index: %d, string length %d\n", idx, mplxBits.size()));

	return mplxBits[findIdx].iVal;
}

void ParityplexWorld::createInstance(InstInfo& t, int set) {
	PtInstInfo& bt = static_cast<PtInstInfo&>(t);

	// make random bit string
	vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	bt._features.resize(fTypes.size());
	for (int i=0; i<fTypes.size(); ++i)
		bt._features[i].iVal = _RandOf(2);

	// find solution
	bt._currentClass = getClassObj(evaluateAns(bt));
	_trainIdx++;
}




} // namespace
