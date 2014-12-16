/*
 * GradCompFragment.h
 *
 *  Created on: 02/05/2014
 *      Author: anthony
 */

#ifndef GRADCOMPFRAGMENT_H_
#define GRADCOMPFRAGMENT_H_

namespace pointrep {

class GradCompFragment: public PtCompFragment {
public:
	GradCompFragment(const vector<Fragment*>& elements,
			PtHarness* parent, int op, float biasVal = 0.0
			);
	virtual ~GradCompFragment();
   	virtual bool operator==(const CompFragment& frag);

   	virtual void initialise(InstInfo& t);
	virtual bool findActivation(InstInfo& t) const; // thread focused method
	virtual void display(ostream&) const;
protected:
	void initRnd(InstInfo& t);
	void initThresh(InstInfo& t);

};


} /* namespace pointrep */

#endif /* GRADCOMPFRAGMENT_H_ */
