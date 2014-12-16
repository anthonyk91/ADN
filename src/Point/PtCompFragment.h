/*
 * PtCompFragment.h
 *
 *  Created on: 19/02/2014
 *      Author: anthony
 */

#ifndef PTCOMPFRAGMENT_H_
#define PTCOMPFRAGMENT_H_

namespace pointrep {

enum {
	opAnd = 1,
	opOr = 2,
	opNand = 3,
	opNor = 4
};

class PtCompFragment : public CompFragment {
public:
	PtCompFragment(const vector<Fragment*>& elements,
			PtHarness* parent, int op, float biasVal = 0.0, bool linkChildren = true);
	virtual ~PtCompFragment();
   	virtual bool operator==(const CompFragment& frag);

	virtual bool findActivation(InstInfo& t) const; // thread focused method
   	virtual void initialise(InstInfo& t);

	virtual void display(ostream&) const;
	virtual void reset();

	void getMergedString(tr1::unordered_map<int, pair<FValue,FValue> >& mergeString) const;

    virtual void write(ostream&) const;
    virtual void read(istream&);

protected:
	int _op;
};


}
#endif
