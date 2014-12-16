/*
 * GradHarness.cpp
 *
 *  Created on: 27/05/2014
 *      Author: anthony
 */


#include "PointRep.h"

namespace pointrep {


double GradHarness::_THRESHSUMVAL = 0.0;

GradHarness::GradHarness() {
	switch (Params::_neurMethod) {
	case neurtanh:
		_THRESHSUMVAL = atanh(PointRep::_brInstance->_matchThreshold);
		break;
	case neurrelu:
	case neurlin:
		_THRESHSUMVAL = PointRep::_brInstance->_matchThreshold;
		break;
	case neurlog:
		// logit function
		_THRESHSUMVAL = - log((1.0 / PointRep::_brInstance->_matchThreshold) - 1.0);
		break;
	}
	_regInt(_gradCreateMethod, IKEY_GRADCREATEMETHOD);
}
GradHarness::~GradHarness() {}

CompFragment* GradHarness::makeCompFragment(vector<Fragment*>& chosenFragments, int op) const
{
	return new GradCompFragment(chosenFragments, const_cast<GradHarness*>(this), op);
}

Fragment* GradHarness::makeAtomFragment() const
{
	return new GradFragment(const_cast<GradHarness&>(*this));
}



void GradHarness::createAtomFragments(InstInfo& t, vector<Fragment*>* newFrags) const
{
	// create new fragment from given world
	GradFragment* newFrag = new GradFragment(const_cast<GradHarness&>(*this));
	if (newFrag) {
		newFrag->createFeatures(t);
		t._newAtoms.push_back(newFrag);
		if (newFrags) newFrags->push_back(newFrag);
	}
}

} // namespace
