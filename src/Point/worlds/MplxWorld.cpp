/*
 * MplxWorld.cpp
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#include "../PointRep.h"

namespace pointrep {

MplxWorld::MplxWorld() : PointWorld() {
	_regInt(_mplxSize, IKEY_MPLXSIZE); //atoi(parameters[sizeParam].c_str());

	_binaryClass = true;
	_epochSize = 10000;

	_classes.resize(2);
	_classes[0] = getClassObj(0);
	_classes[1] = getClassObj(1);

	updateBits();
}

MplxWorld::~MplxWorld() {
	// TODO Auto-generated destructor stub
}

int MplxWorld::remainingInsts(int set) const {
	return _epochSize - _trainIdx;
}

void MplxWorld::start(int set) {
	_trainIdx = 0;
}

void MplxWorld::updateBits() {
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
	int size = _dataBits + _addressBits;

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

void MplxWorld::setDataBits(int dBits) {
	// work out number of address bits
	_dataBits = dBits;
	_addressBits = findEncodingBits(dBits);

	// specify bit string lengths
	int size = _dataBits + _addressBits;

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


//void MplxWorld::genBitString() const {} // already encoded

int MplxWorld::evaluateAns(PtInstInfo& t) const { return evaluateAns(t._features); }
int MplxWorld::evaluateAns(const vector<FValue>& bitString) const {
	// find index from address string
	int idx = bin2Int(bitString, 0, _addressBits);

	// look up index
	unsigned int findIdx = _addressBits + idx;
	_Assert(findIdx <= bitString.size(), ("invalid mplx index: %d, string length %d\n", idx, findIdx));

	return bitString[findIdx].iVal;
}

void MplxWorld::createInstance(InstInfo& t, int set) {
	PtInstInfo& bt = static_cast<PtInstInfo&>(t);

	// make random bit string
	vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	bt._features.resize(fTypes.size());
	//for (int i=0; i<_addressBits; ++i) _bitString.push_back(_RandOf(2));
	//for (int i=0; i<_dataBits; ++i) _bitString.push_back(_RandOf(2));
	for (int i=0; i<fTypes.size(); ++i)
		bt._features[i].iVal = _RandOf(2);

	// find solution
	bt._currentClass = _classes[evaluateAns(bt)];
	_trainIdx++;
}


} // namespace
