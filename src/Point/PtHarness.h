/*
 * PtFragHarness.h
 *
 *  Created on: Nov 5, 2010
 *      Author: anthony
 */

#ifndef PTFRAGHARNESS_H_
#define PTFRAGHARNESS_H_

//#include "../imgrid/ImGrid.h"

namespace pointrep {
//using namespace ImGrid;

#define IKEY_LOGICMETHOD "logicMethod"
#define IKEY_FRAGWEIGHT "weightMethod"
#define IKEY_CREATEMETHOD "createMethod"
#define FKEY_WEIGHTBIAS "weightBias"
#define FKEY_MINWEIGHT "minWeight"
#define FKEY_ORPROB "orProbability"
#define FKEY_ATOMRATIO "atomRatio"

enum {
	logicAnd = 1,
	logicMult = 2,
	logicAndOr = 3
};
enum {
	weightEven = 1,
	weightAcc = 2,
	weightAssoc = 3,
	weightPrefFreq = 4,
	weightAssocNorm = 5,
	weightAvNorm = 6,
};

enum {
	createSingle = 1,
	createMany = 2,
	createSingleAll = 3 // create from all elements not just active
};

class PtHarness : public Harness {
public:
	PtHarness();
	virtual ~PtHarness();

    virtual void postClassification(InstInfo& t) const; // post action-selection (tidy-up)

	virtual void createRndPop();
	virtual void display(ostream&) const;
	virtual void storePop(float ratio); //< moves population of comp frags to kept store

	int getLogicMethod() const;

protected:
	virtual void createAtomFragments(InstInfo& t, vector<Fragment*>* newFrags = NULL) const;
	virtual void createCompFragments(InstInfo& t, vector<Fragment*>* newFrags = NULL) const;

	virtual Fragment* makeAtomFragment() const;
	virtual CompFragment* makeCompFragment(vector<Fragment*>& chosenFragments, int op) const;

	virtual Fragment* readAtom(istream&);
	virtual CompFragment* readComp(istream&);

	int _logicMethod;
	int _weightMethod;
	int _createMethod;
	double _weightBias;
	set<Fragment*> _keptFrags;
	float _orProb;

	set<Fragment*> _preservedFrags; // children of 'kept'
	double _minWeight;
	double _atomRatio;
};

}

#endif
