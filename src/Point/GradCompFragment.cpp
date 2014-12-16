/*
 * GradCompFragment.cpp
 *
 *  Created on: 02/05/2014
 *      Author: anthony
 */

#include "PointRep.h"
#include "Combinatrix.h"

namespace pointrep {

#define MINBUF 0.01
#define MINTHRESH 0.001
GradCompFragment::GradCompFragment(const vector<Fragment*>& elements,
	PtHarness* parent, int op, float biasVal)
	: PtCompFragment(elements, parent, op, biasVal, false)
{
}

GradCompFragment::~GradCompFragment() {
	// TODO Auto-generated destructor stub
}

void GradCompFragment::initialise(InstInfo& t)
{
	int method = static_cast<GradHarness*>(_parHarness)->_gradCreateMethod;
	if (method == gradThresh) initThresh(t);
	else if (method == gradRandom) initRnd(t);
}

void GradCompFragment::initRnd(InstInfo& t) {
	// initialise weights randomly
	FragMap::iterator iter;
	_foreach(_childMap, iter) _childWeights[iter->first]._val = _gRand(SMRNDSD);
	_biasParam._val = _gRand(SMRNDSD);

	// find current activation
	double act, sum = 0.0;
	_foreach(_childMap, iter) {
		sum += _childWeights[iter->first]._val * t._fragDetails[iter->second]._activation;
	}
	act = NeurFn(sum + _biasParam._val);

	// adjust so gives positive response, and no more than 10% of range over threshold
	int ttl = MAXADJITER;
	double target = (Params::_MAXTARGET - PointRep::_brInstance->_matchThreshold) / 2.0;
	double margin = (target - PointRep::_brInstance->_matchThreshold) / 2.0;
	double delta = act - target;
	while (fabs(delta) > margin && ttl--) {
		//double delta = act - Params::_MAXTARGET;
		double grad = delta * NeurDelta(act);
		updateWts(grad, _biasParam, FASTLR);
		_foreach(_childMap, iter) {
			updateWts(grad * t._fragDetails[iter->second]._activation, _childWeights[iter->first], FASTLR);
		}

		// find output again
		sum = 0.0;
		_foreach(_childMap, iter) {
			sum += _childWeights[iter->first]._val * t._fragDetails[iter->second]._activation;
		}
		act = NeurFn(sum + _biasParam._val);
		delta = act - target;
	}
}

void GradCompFragment::initThresh(InstInfo& t) {
	// initialise weights, set so requires all to be active

	FragMap::iterator iter;
	double obsum = 0.0;
	_foreach(_childMap, iter) {
		obsum += t._fragDetails[iter->second]._activation;
	}

	if (_childMap.empty()) return;

	// set match threshold so unit activation (before nonlin)
	// given when each child is at match threshold
	double thresh = PointRep::_brInstance->_matchThreshold;
	double n = _childMap.size();

	// set threshold as halfway between n children
	// active and n-1 active + 1 inactive.  base on activation
	// threshold for child elements
	double acsum = n * thresh;
	double buf = obsum - acsum; //thresh / 2.0; // n * t - (n-1) * t
	buf = _max(MINBUF, buf);

	double c = buf / (1.0 - GradHarness::_THRESHSUMVAL);
	double product = 1.0 / c;
	_foreach(_childMap, iter) {
		_childWeights[iter->first] = product;
	}
	//_biasParam._val = 0.0;
	_biasParam._val = (-obsum / c) + 1.0;

}

bool GradCompFragment::operator==(const CompFragment& frag)
{
	// check for exact equality, which can be useful for
	// early stages after frags are initialised & before BP
	// is introduced
	if (! PtCompFragment::operator==(frag)) return false;
	const GradCompFragment& gfrag = static_cast<const GradCompFragment&>(frag);

	// check if weights are the same

	// check for combinations of child elements, allow variations to order/index
	// populate combination mechanism
	int numEls = _childMap.size();
	Combinatrix<Fragment*> cmb(numEls);
	vector<Fragment*> thisChildList, otherChildList;
	vector<int> thisIndexes, otherIndexes;
	FragMap::const_iterator cIter, oIter;
	_foreach(_childMap, cIter) {
		thisChildList.push_back(cIter->second);
		thisIndexes.push_back(cIter->first);
	}
	_foreach(gfrag._childMap, oIter) {
		otherChildList.push_back(oIter->second);
		otherIndexes.push_back(oIter->first);
	}
	for(int idx = 0; idx < numEls; ++idx) cmb.setCombination(otherChildList, idx);


	// examine each combination (set) of children
	Combinatrix<Fragment*>::exsel_iterator exselIter;
	bool cmbFound;
	for(exselIter = cmb.exsel_begin(); exselIter != cmb.exsel_end(); ++exselIter) {
		vector<Fragment*> selections = *exselIter;

		cmbFound = true;
		for(int idx = 0; idx < numEls; ++idx) {
			// check if this pair of children matches (object (pointer) comparison)
			if (thisChildList[idx] != selections[idx]
				|| (_childWeights.find(thisIndexes[idx])->second._val != gfrag._childWeights.find(otherIndexes[idx])->second._val)) {
				cmbFound = false;
				break;
			}
		}
		if (cmbFound) break;
	}
	if (! cmbFound) return false;

	return false;
}

void GradCompFragment::display(ostream& os) const
{
	CompFragment::display(os);

	// just display weights to each child
	FragMap::const_iterator iter;
	os << "{";
	bool first = true;
	_foreach(_childMap, iter) {
		if (first) first = false;
		else os << ",";

		os << iter->second->_fragID << ":";
		os << "m=" << _childWeights.find(iter->first)->second._val;
	}
	os << ",b=" << _biasParam._val;
	os << ",u:" << getAssocUpdates();
	os << "}";
}

bool GradCompFragment::findActivation(InstInfo& t) const {
	int matchValue;
	double activation;

	// find activation value
	double sum = 0.0;
	FragMap::const_iterator cIter;
	_foreach(_childMap, cIter) {
		sum += t._fragDetails[cIter->second]._activation * _childWeights.find(cIter->first)->second._val;
	}
	activation = NeurFn(sum + _biasParam._val);

    // decrease activation values if dropout is in use and we
    // are in testing mode (freeze)
	// (disused, decrease value at time of access from frag
	// match value)
    /*if (_parHarness->_freezeDevelopment && _parHarness->_dropoutFrags) {
    	_activation *= (1.0 - _parHarness->_dropoutCompRate);
    }*/

	matchValue =
		(activation >= PointRep::_brInstance->_matchThreshold) ? 1 : 0;

	t._fragDetails[this]._match = matchValue;
	t._fragDetails[this]._activation = activation;

	return matchValue;
}

} /* namespace */
