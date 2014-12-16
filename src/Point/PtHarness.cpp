/*
 * PtFragHarness.cpp
 *
 *  Created on: Nov 5, 2010
 *      Author: anthony
 */

#include "PointRep.h"
#include <math.h>

namespace pointrep {


PtHarness::PtHarness()
{
	_regInt(_logicMethod, IKEY_LOGICMETHOD);
	_regInt(_weightMethod, IKEY_FRAGWEIGHT);
    if (_weightMethod == weightAvNorm) _calcAvNorm = true;

	_regIntOp(_createMethod, IKEY_CREATEMETHOD, createSingle);
	_regDblCond(_weightBias, FKEY_WEIGHTBIAS, 0.0, (_weightMethod != weightEven));
	_regDblCond(_minWeight, FKEY_MINWEIGHT, 0.0, (_weightMethod != weightEven));
	_regFltCond(_orProb, FKEY_ORPROB, 0.0, (_logicMethod == logicAndOr));
	_regDbl(_atomRatio,"atomRatio");

	_keepChildOrder = false;


}

PtHarness::~PtHarness() {
	set<Fragment*>::iterator iter;
	_foreach(_keptFrags, iter) {
		(*iter)->removeParents();
		delete *iter;
	}
}

void PtHarness::display(ostream& os) const
{
	Harness::display(os);

	os << " kept:" << _keptFrags.size();
}

void PtHarness::postClassification(InstInfo& t) const
{
	Harness::postClassification(t);

	// reset activity of all kept frags
	/*set<Fragment*>::iterator kIter;
	_foreach(_keptFrags, kIter) {
		BinCompFragment* keptFrag = static_cast<BinCompFragment*>(*kIter);
		keptFrag->reset();
	}*/
}


// instantiateCompFrag from ImFragHarness?

#define MINFACTOR 0.0001
void PtHarness::createCompFragments(InstInfo& t, vector<Fragment*>* newFrags) const
{
	// simply grab a number of random fragments or other composites
	// and use them to create a new composite

	// build composite from activated fragments

	// randomise order to prevent search order bias
	vector<Fragment*> chosenFragments;

	unsigned int nChoose = _RandOf(_maxChildren-1) + 2;

	int op;
	if (_logicMethod == logicAnd) op = opAnd;
	else if (_logicMethod == logicAndOr) {
		//op = _RandOf(2) + 1;
		if (_pRand(_orProb)) op = opOr;
		else op = opAnd;
	}
	else warn("logic method not implemented in comp create\n");

	bool useAll = false;
	if (op == opOr) useAll = true;

	if (chosenFragments.size() < nChoose) {
		vector<Fragment*> searchFragments, storedFrags;
		int ftSize = PointRep::_brInstance->_featureTypes.size();
		if (useAll) {
			//searchFragments.resize(_atomFragments.size() + _compFragments.size());
			vector<Fragment*>::const_iterator lIter;
			_foreach(_atomFragments, lIter) {
				// check if within size range of current world
				PtFragment* thisFrag = static_cast<PtFragment*>(*lIter);
				if (thisFrag->getMaxIndex() >= ftSize) continue;

				searchFragments.push_back(thisFrag);
			}
			searchFragments.insert(searchFragments.end(), _compFragments.begin(), _compFragments.end());
		} else {
			// just create from active
			searchFragments.assign(t._activeFragments.begin(), t._activeFragments.end());
		}

		// find 'active' kept frags, assume receives bottom-up activation call
		set<Fragment*>::iterator kIter;
		_foreach(_keptFrags, kIter) {
			PtCompFragment* keptFrag = static_cast<PtCompFragment*>(*kIter);

			// include if active
			if (useAll || t._fragDetails[keptFrag]._match) storedFrags.push_back(keptFrag);
		}

		if (searchFragments.size() + storedFrags.size() < nChoose) {
			if (nChoose < 2) return; // not enough frags
			nChoose = searchFragments.size() + storedFrags.size();
		}

		vector<int> sIndexes, kIndexes; // chosen indexes
		if (_weightMethod == weightEven) {
			while (chosenFragments.size() < nChoose) {
				int pos;
				if (storedFrags.empty() || kIndexes.size() >= storedFrags.size() || _pRand(0.5)) {
					while (true){ // should exit
						pos = _RandOf(searchFragments.size());
						if (! _findInVec(sIndexes, pos)) break;
					}
					sIndexes.push_back(pos);
					chosenFragments.push_back(searchFragments[pos]);
				} else {
					while (true) { // should exit
						pos = _RandOf(storedFrags.size());
						if (! _findInVec(kIndexes, pos)) break;
					}
					kIndexes.push_back(pos);
					chosenFragments.push_back(storedFrags[pos]);
				}
			}
		} else {
			// weighted selection
			vector<Fragment*>* vec;
			vector<int>* indexes;

			//findAssocNorms(); // update values from parents
			while (chosenFragments.size() < nChoose) {
				if (storedFrags.empty() || kIndexes.size() >= storedFrags.size() || _pRand(0.5)) {
					vec = &searchFragments;
					indexes = &sIndexes;
				}
				else {
					vec = &storedFrags;
					indexes = &kIndexes;
				}

				Fragment* chosen = NULL;
				int chIdx;

				double tot_wt = 0.0;
				for (unsigned int idx = 0; idx < vec->size(); ++idx) {
					// check if already allocated
					if (_findInVec(*indexes, idx)) continue;


					Fragment* thisFragment = vec->at(idx);

					double wt = 1.0;
					//if (_weightMethod == weightEven)
					if (_weightMethod == weightAssoc) {
						// find largest assoc weight
						AssocMap::const_iterator iter;
						wt = thisFragment->getMaxAssociation();
					} else if (_weightMethod == weightAssocNorm) {
						// assume updated recently (?)
						wt = thisFragment->getAssocNorm();
					}

					wt = _max(wt, _minWeight); // give minimum weight*/
					double biasedWt = _weightBias * wt;
					if (biasedWt > MAXEXP) {
						biasedWt = MAXEXP;
					}

					wt = exp(biasedWt);

					tot_wt += wt;
					if (! chosen || _pRand(wt / tot_wt)) {
						chosen = thisFragment;
						chIdx = idx;
					}
				}
				if (chosen) {
					chosenFragments.push_back(chosen);
					indexes->push_back(chIdx);
				} else return;
			}
		}
	}

	// create composite from chosen fragments
	PtCompFragment* newComposite = NULL;
	if (chosenFragments.size() >= 2) { // minimum 2 to make composite
		newComposite = static_cast<PtCompFragment*>(makeCompFragment(chosenFragments, op));
		if (newComposite) {
			newComposite->initialise(t);
			t._newComps.push_back(newComposite);

			//cout << "created comp " << newComposite << " on batch " << &t << endl;
			//newComposite->display(cout);
			if (newFrags) newFrags->push_back(newComposite);
		}
	}
}

CompFragment* PtHarness::makeCompFragment(vector<Fragment*>& chosenFragments, int op) const
{
	// create but don't link to children yet
	return new PtCompFragment(chosenFragments, const_cast<PtHarness*>(this), op, 0.0, false);
}

Fragment* PtHarness::makeAtomFragment() const
{
	return new PtFragment(const_cast<PtHarness&>(*this));
}


void PtHarness::createAtomFragments(InstInfo& t, vector<Fragment*>* newFrags) const {
	// create new fragment from given world
	PtFragment* newFrag = static_cast<PtFragment*>(makeAtomFragment());
	if (newFrag) {
		newFrag->createFeatures(t);
		t._newAtoms.push_back(newFrag);

		//cout << "created atom " << newFrag << " on batch " << &t << endl;
		//newFrag->display(cout);
		if (newFrags) newFrags->push_back(newFrag);
	}
}

void PtHarness::storePop(float ratio) {
	// keep existing 'kept' frags, and add to collection.

	// now clear compfrags store, removing elements not kept
	// just keep better half of store
	if (_fragPopMethod == assocNorm) {
		findAssocNorms();

		// sort fragments on assocNorm value
		sort(_compFragments.begin(), _compFragments.end(), Fragment::assocNormGThan);
	} else {
		warn("pop method not recognised in storePop\n");
	}

	int keepidx = _numComposites * ratio;
	for (int idx = 0; idx < _compFragments.size(); ++idx) {
		CompFragment* thisComp = _compFragments[idx];
		if (idx <= keepidx) {
			static_cast<PtCompFragment*>(thisComp)->_storedFrag = true;
			// remove all links from children to this
			_keptFrags.insert(thisComp);
		} else {
			thisComp->removeParents(); // unlink as child of each parent
			_numAssocs -= thisComp->getAssociationSet().size();
			_idMap.erase(thisComp->_fragID);
			delete thisComp;
		}
	}
	_compFragments.clear();
	_numComposites = 0;
}


int PtHarness::getLogicMethod() const
{
	return _logicMethod;
}

void PtHarness::createRndPop() {
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	const vector<int>& nClasses = PointRep::_brInstance->_featureClasses;

	for (unsigned int pos=0; pos < fTypes.size(); ++pos) {
		if (fTypes[pos] != iClass) continue;
		for (int v=0; v<nClasses[pos]; ++v) {
			//bs[pos] = v;

			if (_numAtoms >= _maxAtomFragments) break;

			PtFragment* newFrag = static_cast<PtFragment*>(makeAtomFragment());
			newFrag->clearValues();
			FValue thisVal;
			thisVal.iVal = v;
			newFrag->addValue(pos,thisVal, eq);

			if (! addFragment(newFrag)) {
				newFrag->removeParents();
				delete newFrag;
			}
		}
	}
}

Fragment* PtHarness::readAtom(istream& in) {
	PtFragment* newFrag = static_cast<PtFragment*>(makeAtomFragment());
	newFrag->clearValues();

	newFrag->read(in);

	return newFrag;
}
CompFragment* PtHarness::readComp(istream& in)
{
	vector<Fragment*> dummy;
	PtCompFragment* newFrag = static_cast<PtCompFragment*>(makeCompFragment(dummy, 0));

	newFrag->read(in);

	return newFrag;
}


} //namespace
