/*
 * PtFragment.h
 *
 *  Created on: Nov 5, 2010
 *      Author: anthony
 */

#ifndef PTFRAGMENT_H_
#define PTFRAGMENT_H_

//#include "../imgrid/ImGrid.h"

namespace pointrep {
//using namespace ImGrid;

enum {
	other = 0,
	ge = 1,
	le = 2,
	eq = 3,
	gt = 4,
	lt = 5,
	ne = 6
};


class PtFragment : public Fragment {
public:
	PtFragment(PtHarness& parent); // creates fragment without specifying properties
	virtual ~PtFragment();

	virtual bool operator==(const Fragment& frag); // assume BinFragment
	virtual bool findActivation(InstInfo& t) const;

	virtual void createFeatures(InstInfo& t, bool forceRnd = false);

	virtual void clearAllAssociations();

	virtual void display(ostream&) const;
	void getOpMap(tr1::unordered_map<int,int>& opmap) const;
	virtual void clearValues();

	virtual void reset();
	int getMaxIndex() const;
	void addValue(int idx, FValue val, int op);
	void getVals(tr1::unordered_map<int,FValue>& valmap) const;

	void setStored(bool stored);

    virtual void write(ostream&) const;
    virtual void read(istream&);

protected:

	// change to single position/value
	vector<int> _indexes;
	vector<FValue> _values;
	vector<int> _operations; // <,>,=

	bool _storedFrag;
};


} // namespace

#endif
