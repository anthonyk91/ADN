/*
 * PtCompFragment.cpp
 *
 *  Created on: 19/02/2014
 *      Author: anthony
 */

#include "PointRep.h"

namespace pointrep {



PtCompFragment::PtCompFragment(const vector<Fragment*>& elements,
		PtHarness* parent, int op, float biasVal, bool linkChildren)
	: CompFragment(elements, parent, biasVal, linkChildren)
{
	_op = op;
}

PtCompFragment::~PtCompFragment() {
}

bool PtCompFragment::operator==(const CompFragment& frag)
{
	return CompFragment::operator==(frag) && _op == static_cast<const PtCompFragment&>(frag)._op;
}

bool PtCompFragment::findActivation(InstInfo& t) const
{

	// selective activation according to frag type
	int matchValue;

	FragMap::const_iterator cIter;
	if (_op == opAnd) {
		matchValue = 1;
		_foreach(_childMap, cIter) {
			// assume match test of children will be finished before reaching here
			//if(! cIter->second->getMatchesFound()) return 0;
			int matchVal = t._fragDetails[cIter->second]._match;

			if (! matchVal) {
				matchValue = 0;
				break;
			}
		}

	} else if (_op == opOr) {
		matchValue = 0;
		_foreach(_childMap, cIter) {
			int matchVal = t._fragDetails[cIter->second]._match;

			if (matchVal) {
				matchValue = 1;
				break;
			}
		}
	} else {
		warn("logic method not implemented\n");
		return 0;
	}

	// possibly restrict recording of active frag if 'stored'

	t._fragDetails[this]._match = matchValue;
	t._fragDetails[this]._activation = matchValue;

	return matchValue;
}


void PtCompFragment::reset()
{
	CompFragment::reset();
}


void PtCompFragment::display(ostream& os) const
{
	CompFragment::display(os);

	tr1::unordered_map<int, pair<FValue,FValue> > mergeMap;
	getMergedString(mergeMap);
	tr1::unordered_map<int, pair<FValue,FValue> >::iterator iter;

	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	const vector<pair<FValue, FValue> >& fRanges = PointRep::_brInstance->_featureRanges;

	os << "{";
	bool first = true;
	_foreach(mergeMap, iter) {
		if (first) first = false;
		else os << ",";

		int i=iter->first;
		os << i << ":";

		if (fTypes[i] == iClass) {
			if (mergeMap[i].first.iVal == mergeMap[i].second.iVal)
				os << mergeMap[i].first.iVal;
			else
				os << "X";
			continue;
		} else if (fTypes[i] == iSeq) {
			int rngMin = fRanges[i].first.iVal;
			int rngMax = fRanges[i].second.iVal;

			if (mergeMap[i].first.iVal > mergeMap[i].second.iVal) {
				os << "_";
				continue;
			}
			if (mergeMap[i].first.iVal == rngMin) {
				os << "<=" << mergeMap[i].second.iVal;
			} else if (mergeMap[i].second.iVal == rngMax) {
				os << ">=" << mergeMap[i].first.iVal;
			} else os << mergeMap[i].first.iVal << "-" << mergeMap[i].second.iVal;
		} else if (fTypes[i] == fSeq) {
			float rngMin = fRanges[i].first.fVal;
			float rngMax = fRanges[i].second.fVal;

			if (mergeMap[i].first.fVal > mergeMap[i].second.fVal) {
				os << "_";
				continue;
			}
			if (mergeMap[i].first.fVal == rngMin) {
				os << "<=" << mergeMap[i].second.fVal;
			} else if (mergeMap[i].second.fVal == rngMax) {
				os << ">=" << mergeMap[i].first.fVal;
			} else os << mergeMap[i].first.fVal << "-" << mergeMap[i].second.fVal;
		}
	}
	os << "}";

	os << " op:" << _op << " ";

	os << "u:" << getAssocUpdates();
}

void PtCompFragment::write(ostream& out) const
{
	CompFragment::write(out);

	out << " " << (_storedFrag ? 1 : 0) << " " << _op;
}
void PtCompFragment::read(istream& in)
{
	CompFragment::read(in);
	in >> _storedFrag >> _op;
}

void PtCompFragment::initialise(InstInfo& t)
{

}

void PtCompFragment::getMergedString(tr1::unordered_map<int, pair<FValue,FValue> >& mergeString) const
{
	// find merged representation

	FragMap::const_iterator cIter;
	//mergeString.clear();
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	const vector<pair<FValue, FValue> >& fRanges = PointRep::_brInstance->_featureRanges;

	_foreach(_childMap, cIter) {
		// get string from frag
		tr1::unordered_map<int, pair<FValue,FValue> > thisRng;
		if (cIter->second->isComposite()) {
			static_cast<PtCompFragment*>(cIter->second)->getMergedString(thisRng);
		} else {
			tr1::unordered_map<int,FValue> valmap;
			tr1::unordered_map<int,int> opmap;
			PtFragment* frag = static_cast<PtFragment*>(cIter->second);
			frag->getVals(valmap);
			frag->getOpMap(opmap);
			tr1::unordered_map<int, FValue>::iterator vIter;
			_foreach(valmap, vIter) {
				int idx = vIter->first;
				int op = opmap[idx];
				thisRng[idx] = fRanges[idx];
				if (fTypes[idx] == iClass) {
					thisRng[idx].first.iVal = vIter->second.iVal;
					thisRng[idx].second.iVal = vIter->second.iVal;
				} else if (fTypes[idx] == iSeq) {
					int val = vIter->second.iVal;
					if (op == eq) { thisRng[idx].first.iVal = val; thisRng[idx].second.iVal = val;}
					else if (op == ge) thisRng[idx].first.iVal = val;
					else if (op == le) thisRng[idx].second.iVal = val;
				} else if (fTypes[idx] == fSeq) {
					float val = vIter->second.fVal;
					if (op == eq) { thisRng[idx].first.fVal = val; thisRng[idx].second.fVal = val;}
					else if (op == ge) thisRng[idx].first.fVal = val;
					else if (op == le) thisRng[idx].second.fVal = val;
				}
			}
		}

		if (mergeString.empty()) {
			tr1::unordered_map<int, pair<FValue,FValue> >::iterator mIter;
			_foreach(thisRng, mIter) {
				mergeString[mIter->first] = mIter->second;
			}
			continue;
		}

		//merge

		if (_op == opAnd) {
			tr1::unordered_map<int, pair<FValue,FValue> >::iterator iter;
			_foreach(thisRng, iter) {
				int idx = iter->first;
				if (fTypes[idx] == iClass) {
					if (mergeString.find(idx) == mergeString.end()) {
						mergeString[idx] = iter->second;
					} else {
						mergeString[idx].first.iVal = _min(mergeString[idx].first.iVal, iter->second.first.iVal);
						mergeString[idx].second.iVal = _max(mergeString[idx].second.iVal, iter->second.second.iVal);
					}
				} else if (fTypes[idx] == iSeq) {
					// check if value is defined in merge string
					int rngMin = fRanges[idx].first.iVal;
					int rngMax = fRanges[idx].second.iVal;
					if (mergeString.find(idx) == mergeString.end()) {
						mergeString[idx].first.iVal = rngMin;
						mergeString[idx].second.iVal = rngMax;
					}
					int& as = mergeString[idx].first.iVal;
					int& al = mergeString[idx].second.iVal;
					int bs = iter->second.first.iVal;
					int bl = iter->second.second.iVal;

					if (bs != rngMin) {
						if (as == rngMin || bs > as) as = bs;
					}
					if (bl != rngMax) {
						if (al == rngMax || bl < al) al = bl;
					}
				} else if (fTypes[idx] == fSeq) {
					// check if value is defined in merge string
					float rngMin = fRanges[idx].first.fVal;
					float rngMax = fRanges[idx].second.fVal;
					if (mergeString.find(idx) == mergeString.end()) {
						mergeString[idx].first.iVal = rngMin;
						mergeString[idx].second.iVal = rngMax;
					}
					float& as = mergeString[idx].first.fVal;
					float& al = mergeString[idx].second.fVal;
					float bs = iter->second.first.fVal;
					float bl = iter->second.second.fVal;

					if (bs != rngMin) {
						if (as == rngMin || bs > as) as = bs;
					}
					if (bl != rngMax) {
						if (al == rngMax || bl < al) al = bl;
					}
				}
			}
		} else if (_op == opOr) {
			warn("opOr display not implemented");
			continue;
		}
	}
}


}
