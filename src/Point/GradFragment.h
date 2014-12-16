/*
 * GradFragment.h
 *
 *  Created on: 27/05/2014
 *      Author: anthony
 */

#ifndef GRADFRAGMENT_H_
#define GRADFRAGMENT_H_

namespace pointrep {

#define SMRNDSD 0.01 // random weight generation
#define MAXADJITER 10

class GradFragment: public PtFragment {
public:
	GradFragment(PtHarness& parent);
	virtual ~GradFragment();

	virtual bool operator==(const Fragment& frag); // assume RngFragment
	virtual bool findActivation(InstInfo& t) const; // thread focused method

	virtual void createFeatures(InstInfo& t, bool forceRnd = false);

	virtual void display(ostream&) const;
	//NeurParam& getWeightParam();

	virtual void doBackprop(InstInfo& t, double delta);

    virtual void write(ostream&) const;
    virtual void read(istream&);

protected:
	vector<NeurParam> _weightVal;
	void initThresh(InstInfo& t, bool forceRnd);
	void initRnd(InstInfo& t);
};

} // namespace

#endif /* GRADFRAGMENT_H_ */
