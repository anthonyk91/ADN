/*
 * GradHarness.h
 *
 *  Created on: 27/05/2014
 *      Author: anthony
 */

#ifndef GRADHARNESS_H_
#define GRADHARNESS_H_

#define IKEY_GRADCREATEMETHOD "gradCreateMethod"

namespace pointrep {

enum {
	gradThresh = 1,
	gradRandom = 2
};

class GradHarness : public PtHarness {
public:
	GradHarness();
	virtual ~GradHarness();

	virtual void createAtomFragments(InstInfo& t, vector<Fragment*>* newFrags = NULL) const;
	static double _THRESHSUMVAL;

	int _gradCreateMethod;

protected:
	virtual Fragment* makeAtomFragment() const;
	virtual CompFragment* makeCompFragment(vector<Fragment*>& chosenFragments, int op) const;

};


}


#endif /* GRADHARNESS_H_ */
