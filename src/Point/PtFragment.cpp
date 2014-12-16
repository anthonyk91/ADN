/*
 * PtFragment.cpp
 *
 *  Created on: Nov 5, 2010
 *      Author: anthony
 */

#include "PointRep.h"
#include <math.h>

namespace pointrep {

PtFragment::PtFragment(PtHarness& parent)
	: Fragment(&parent)
{
	_storedFrag = false;
}

PtFragment::~PtFragment() {
}

bool PtFragment::operator==(const Fragment& frag) {
	const PtFragment& bFrag = static_cast<const PtFragment&>(frag);

	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	vector<int> bIdxs(bFrag._indexes);
	if (_indexes.size() != bIdxs.size()) return false;
	for (int i=0; i<_indexes.size(); ++i) {
		bool found = false;
		for (int j=0; j<bIdxs.size(); ++j) {
			if (_indexes[i] == bIdxs[j]) {
				if (_operations[i] != bFrag._operations[j]) return false;
				if (fTypes[_indexes[i]] == iSeq || fTypes[_indexes[i]] == iClass) {
					if (_values[i].iVal != bFrag._values[j].iVal) return false;
				} else if (fTypes[_indexes[i]] == fSeq) {
					if (_values[i].fVal != bFrag._values[j].fVal) return false;
				}
				found = true;

				// clear this entry to prevent re-finding it
				bIdxs[j] = -1;
				break;
			}
		}
		if (! found) return false;
	}
	return true;
}


void PtFragment::reset()
{
	Fragment::reset();
	//logout << "atom " << this << " resetting" << endl;
	//writeBuf();
}

void PtFragment::clearAllAssociations()
{
	// when given call to clear associations, clear all count values too
    _statesObs = 0;
    _statesMatch = 0;
    _matchFreq = 0.0;
}

bool PtFragment::findActivation(InstInfo& t) const
{
	// finds all given instances
	PtInstInfo& bt = static_cast<PtInstInfo&>(t);

	const vector<FValue>& rString = bt._features;
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	int matchValue;

	if (! _indexes.empty()) matchValue = 1;
	for (int i=0; i<_indexes.size(); ++i) {
		int idx = _indexes[i];
		if (idx >= rString.size()) {
			matchValue = 0;
			break;
		}

		int featureType = fTypes[idx];
		if (featureType == iClass) {
			// check equality
			if (rString.at(idx).iVal != _values[i].iVal) {
				matchValue = 0;
				break;
			}
		} else if (featureType == iSeq) {
			int iVal = rString.at(idx).iVal;
			int oVal = _values[i].iVal;
			if ((_operations[i] == ge && iVal < oVal) ||
				(_operations[i] == le && iVal > oVal) ||
				(_operations[i] == eq && iVal != oVal) ||
				(_operations[i] == gt && iVal <= oVal) ||
				(_operations[i] == lt && iVal >= oVal) ||
				(_operations[i] == ne && iVal == oVal)) {
					matchValue = 0;
					break;
			}
		} else if (featureType == fSeq) {
			float fVal = rString.at(idx).iVal;
			float oVal = _values[i].iVal;
			if ((_operations[i] == ge && fVal < oVal) ||
				(_operations[i] == le && fVal > oVal) ||
				(_operations[i] == eq && fVal != oVal) ||
				(_operations[i] == gt && fVal <= oVal) ||
				(_operations[i] == lt && fVal >= oVal) ||
				(_operations[i] == ne && fVal == oVal)) {
					matchValue = 0;
					break;
			}
		}
	}

	t._fragDetails[this]._match = matchValue;
	t._fragDetails[this]._activation = matchValue;

	return matchValue;
}


//#define EQ_LE_GE 3
#define GE_LE 2
void PtFragment::createFeatures(InstInfo& t, bool forceRnd) {
	// from the current world state, produce pattern

	int indexes = _RandOf(PointRep::_brInstance->_atomSymbols) + 1;
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	const vector<FValue>& vec = static_cast<PtInstInfo&>(t)._features;
	if (! vec.empty() && ! forceRnd) {
		if (indexes > vec.size()) indexes = vec.size();
		_indexes.resize(indexes);
		_values.resize(indexes);
		_operations.resize(indexes);
		for (int i=0; i<indexes; ++i) {
			int idx = _RandOf(vec.size());
			_indexes[i] = (idx);
			_values[i] = vec.at(idx);

			// choose random operation for this feature according to feature type
			if (fTypes[idx] == iClass) _operations[i] = eq;
			else if (fTypes[idx] == iSeq || fTypes[idx] == fSeq) _operations[i] = _RandOf(GE_LE) + 1;
		}
	} else {
		// world present but not active, generate random
		if (indexes > fTypes.size()) indexes = fTypes.size();
		_indexes.resize(indexes);
		_values.resize(indexes);
		_operations.resize(indexes);
		const vector<pair<FValue,FValue> >& ranges = PointRep::_brInstance->_featureRanges;
		const vector<int>& fClasses = PointRep::_brInstance->_featureClasses;
		for (int i=0; i<indexes; ++i) {
			int idx = _RandOf(fTypes.size());
			_indexes[i] = idx;

			// choose operation and value within range
			if (fTypes[idx] == iClass) {
				_operations[i] = eq;
				_values[i].iVal = _RandOf(fClasses[idx]);
			} else if (fTypes[idx] == iSeq || fTypes[idx] == fSeq) {
				_operations[i] = _RandOf(GE_LE) + 1;
				const pair<FValue,FValue>& rng = ranges[idx];
				if (fTypes[idx] == iSeq) {
					_values[i].iVal = _RandOf(rng.second.iVal - rng.first.iVal) + rng.first.iVal;
				} else if (fTypes[idx] == fSeq) {
					_values[i].fVal = _fRand() * (rng.second.fVal - rng.first.fVal) + rng.first.fVal;
				}
			}
		}
	}
}


void PtFragment::display(ostream& outstream) const {
	Fragment::display(outstream);

	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	for(int i=0; i<_indexes.size(); ++i) {
		if (i != 0) outstream << ",";

		int idx = _indexes[i];
		outstream << idx << ":";
		if (_operations[i] == ge) outstream << ">=";
		else if (_operations[i] == le) outstream << "<=";
		else if (_operations[i] == eq) outstream << "==";
		if (fTypes[idx] == iClass || fTypes[idx] == iSeq) outstream << _values[i].iVal;
		else if (fTypes[idx] == fSeq) outstream << _values[i].fVal;
	}
}

void PtFragment::write(ostream& out) const
{
	Fragment::write(out);

	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	out << " " << _indexes.size();
	for(int i=0; i<_indexes.size(); ++i) {
		int idx = _indexes[i];
		out << " ";
		out << idx << " " << _operations[i];
		if (fTypes[idx] == iClass || fTypes[idx] == iSeq) out << " " << _values[i].iVal;
		else if (fTypes[idx] == fSeq) out << " " << _values[i].fVal;
	}

	out << " " << (_storedFrag ? 1 : 0);

}
void PtFragment::read(istream& in)
{
	Fragment::read(in);

	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	_indexes.clear();
	_operations.clear();
	_values.clear();

	int n;
	in >> n;
	for (int i=0; i<n; ++i) {
		int idx, op;
		in >> idx >> op;

		_indexes.push_back(idx);
		_operations.push_back(op);

		FValue f;
		if (fTypes[idx] == iClass || fTypes[idx] == iSeq) {
			int d;
			in >> d;
			f.iVal = d;
		}
		else if (fTypes[idx] == fSeq) {
			double d;
			in >> d;
			f.fVal = d;
		}
		_values.push_back(f);
	}

	in >> _storedFrag;
}


int PtFragment::getMaxIndex() const {
	int maxIdx;
	for (int i=0; i<_indexes.size(); ++i) {
		if (i==0 || _indexes[i] > maxIdx) maxIdx = _indexes[i];
	}
	return maxIdx;
}
void PtFragment::addValue(int idx, FValue val, int op)
{
	_indexes.push_back(idx);
	_values.push_back(val);
	_operations.push_back(op);
}

void PtFragment::getVals(tr1::unordered_map<int,FValue>& valmap) const {
	// return _bitString;
	for (int i=0; i<_indexes.size(); ++i)
		valmap[_indexes[i]] = _values[i];
}

void PtFragment::clearValues()
{
	_indexes.clear();
	_values.clear();
	_operations.clear();
}

void PtFragment::setStored(bool stored) {
	_storedFrag = stored;
}

void PtFragment::getOpMap(tr1::unordered_map<int,int>& opmap) const {
	for (unsigned int i=0; i<_indexes.size(); ++i)
		opmap[_indexes[i]] = _operations[i];
}



} // namespace
