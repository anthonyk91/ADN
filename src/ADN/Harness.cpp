//#include "harness.h"
#include "ADN.h"
#include <vector>
#include <algorithm>
#include <time.h>
#include <math.h>
#include "Associating.h"

namespace adn {

using namespace std;
using namespace logger;
using namespace utils;

//#define recordChange(frag, grad, param) doRecordChange(t._fragDetails[frag], grad, param)
#define recordChange(frag, grad, param) { t._fragDetails[frag]._paramChanges[param].push_back(grad); }

Harness::Harness()
	: _statCollector(NULL)
{
    _regInt(_removalIterations, IKEY_RMVITER);
    _regFlt(_avResAlpha,IKEY_AVRWDALPHA);

	_regInt(_maxComposites,IKEY_MAXCOMPOSITES);
	_regInt(_maxAtomFragments,IKEY_MAXATOMFRAGMENTS);

    _regFlt(_createEachInstProb,FKEY_CREATEINSTPROB);
    _regFlt(_atomRatio, FKEY_ATOMRATIO);

    /*_regInt(_dropoutFrags,IKEY_DROUPOUTFRAGS);
	_regFltCond(_dropoutObsRate, FKEY_DROUPOUTOBS, 0.0, _dropoutFrags);
	_regFltCond(_dropoutCompRate, FKEY_DROUPOUTCOMPS, 0.0, _dropoutFrags);*/

	_regInt(_insertEqCheck, IKEY_INSERTCHECK);
	_regInt(_maxChildren,IKEY_MAXCHILD);

	_regInt(_testMethod, IKEY_PROPMETHOD);
	//_regIntCond(_trackActivity, IKEY_TRACKACTIVITY, true, ); // only ask for parameter if not using avNorm (which requires it)
	_regIntOp(_trackActivity, IKEY_TRACKACTIVITY, false);
    _regIntOp(_showProcessing, IKEY_SHOWPROC, 0);
    _regIntOp(_runPopAdjust, IKEY_RUNPOPADJUST, true);

    // Association params (move separate?)
    _regInt(_outActivationMethod,INT_OUTACTMETHOD);
    _regInt(_backpropMethod,INT_BACKPROPMETHOD);
    _regInt(_activationSet,INT_ACTIVSET);
    _regInt(_fragPopMethod,INT_FRAGPOPMETHOD);
    if (_fragPopMethod == avNorm) _calcAvNorm = true;
    _regDbl(_probPostAgentParam,FKEY_POSTCREATE);
    _regInt(_minDepth, IKEY_MINDEPTH);
    _regInt(_sparseAssocLimit, IKEY_SPARSELIM);
    _regDbl(_sparseCreateProb, FKEY_SPARSECREATEPROB);
    _regInt(_normError, IKEY_NORMERROR);

    _numAssocs = 0;
    _errUpdates = 0;


    _avResUpdates = 0;
    _averageResult = 0.0;
    _gamesPlayed = 0;
    _freezeDevelopment = false;

	_numAtoms = 0;
	_numComposites = 0;

	_keepChildOrder = true;
	_maxFragID = 0;

	_statCollector = new FragStatCollector(*this);

	resetState();
}

Harness::~Harness()
{
    // remove blocks from freezing development to allow proper shut down of harness and agents
    _freezeDevelopment = false;

	if (_statCollector) delete _statCollector;
}

void Harness::resetState()
{
	_gameMove = 0;
}

void Harness::initialise()
{
	_Assert(_worldPtr, ("no world available"));
	// initialise bias weights if not set
	vector<const Associating*>::iterator iter;
	_foreach(_worldPtr->_allClasses, iter) {
		if (_classBiases.find(*iter) == _classBiases.end())
			_classBiases[*iter] = 0.0;
	}
}


const Associating* Harness::getClassification(InstInfo& t) const
{
	t.reset();

	observeState(t);

	const Associating* thisClass = findOutputActivations(t);

	return thisClass;
}


void Harness::createRndPop()
{
	warn("Harness::createRndPop not implemented");
}




FragStatCollector& Harness::statCollector()
    { return *_statCollector; }

const FragStatCollector& Harness::statCollector() const
    { return *_statCollector; }

/****** protected member methods ***********/


void Harness::observeState(InstInfo& t) const
{

    // activate fragments and rules using appropriate method
    activateFrags(t);

    // from activated fragment set generate new fragments as necessary
    if (! _freezeDevelopment && _runPopAdjust) {
	    if (_pRand(_createEachInstProb * _atomRatio))
	    	createAtomFragments(t);

	    if (_pRand(_createEachInstProb))
	    	createCompFragments(t);
    }

    // record activation stats (not thread safe, but doesn't matter)
    _stats._lastActiveAtoms = t._activeAtomFragments.size();
    _stats._lastActiveComps = t._activeComposites.size();
}

void Harness::postClassification(InstInfo& t) const
{
    // end of turn (post action).  if in training mode, find
	// correct class from world and allocate rewards accordingly
	// to agents stored in action sets

	// get class object for associating
	const Associating* curClass = t._currentClass;

	// check if perform updates/convergence
	if (! _freezeDevelopment) {
		// using gradient descent method.  update assoc
		// weights using backpropagation

		double err = 0.0;

		// find delta value (dE/dsum) for each class
		if (_outActivationMethod == selSum && _worldPtr->getBinaryClass()) {
			// check if correct class is high or low class
			pair<Associating*, Associating*> binClasses = _worldPtr->getBinaryClasses();
			float targval;
			if (curClass == binClasses.first) targval = Params::_MINTARGET;
			else targval = Params::_MAXTARGET;

			Associating* hClass = binClasses.second;
			// get output val, based on second (high) class
			double outval;
			if (t._outputs.find(hClass) == t._outputs.end()) {
				t._outputs[hClass] = Params::_NEUTRVAL;
				outval = Params::_NEUTRVAL;
			} else outval = t._outputs[hClass];
			double delta = NeurDelta(outval) * (outval - targval);
			err = (outval - targval) * (outval - targval);

			t._classDeltas[hClass] = delta;
		} else {
			// multiple classes

			// check if current class is included in activations,
			// add if not
			if (t._outputs.find(curClass) == t._outputs.end()) {
				if (_outActivationMethod == selSoftMax)
					t._outputs[curClass] = 0.0; //1 / float(_classActivations.size());
				else
					t._outputs[curClass] = Params::_NEUTRVAL;
			}

			ClassMap::iterator iter;
			_foreach(t._outputs, iter) {
				const Associating* thisClass = iter->first;
				double outval = iter->second;

				// find delta values for each class
				double thisDelta;
				if (_outActivationMethod == selSoftMax) {
					double targval = SMAXLTARGET;
					if (thisClass == curClass) targval = SMAXHTARGET;
					thisDelta = (outval - targval);
					err += (outval - targval) * (outval - targval);
				} else {
					double targval = Params::_MINTARGET;
					if (thisClass == curClass) targval = Params::_MAXTARGET;
					thisDelta = NeurDelta(outval) * (outval - targval);
					err += (outval - targval) * (outval - targval);
				}
				t._classDeltas[thisClass] = thisDelta;
			}
		}
		t._err = err;

		// normalise deltas according to distribution?
		ClassMap::iterator iter;
		if (_normError) {
			double evenDist = 1 / double(_worldPtr->_allClasses.size());
			_foreach(t._classDeltas, iter) {
				iter->second *= evenDist / _worldPtr->_classDist[iter->first];
			}
		}

		// update class bias(es)
		_foreach(t._classDeltas, iter) {
			t._classBiasMods[iter->first] += iter->second;
		}

		// create new assoc links randomly
		if (_sparseAssocLimit && ! t._activationFrags.empty() && (_numAssocs < _sparseAssocLimit || _pRand(_sparseCreateProb)) ) {
			// probabilistically choose from delta weights
			ClassMap::iterator iter
				= t._classDeltas.begin();
			const Associating* chosenClass = iter->first;
			double wt = fabs(iter->second);
			while (iter != t._classDeltas.end()) {
				double thisWt = fabs(iter->second);
				wt += thisWt;
				if (_pRand(thisWt / wt)) chosenClass = iter->first;
				iter++;
			}

			// choose from frags randomly
			Fragment* addFrag = t._activationFrags[_RandOf(t._activationFrags.size())];
			double grad = t._classDeltas[chosenClass] * t._fragDetails[addFrag]._activation;
			t._fragDetails[addFrag]._newAssocClasses.push_back(chosenClass);
			t._fragDetails[addFrag]._newAssocVals.push_back(grad);
		}

		// update weights from outputs to frags, and through network
		runBackprop(t);

		//bool correctGuess = _chosenClass == t._currentClass;

		// keep track of average result
		/*float resValue = (correctGuess ? 1.0 : 0.0);

		if (t._avResUpdates < MINAVGREWARDSTEPS) {
			t._averageResult = (t._avResUpdates * _averageResult + resValue) / float(t._avResUpdates + 1.0);
			++_avResUpdates;
		} else {
			t._averageResult = t._averageResult * (1.0 - _avResAlpha) + resValue * t._avResAlpha;
		}*/

	}

    /*vector<Fragment*>::const_iterator aIter;
	vector<CompFragment*>::const_iterator cIter;

	if (! _freezeDevelopment && _trackActivity) {
		// record each frag as being part of observed state,
		// to track activity
		_foreach(_atomFragments, aIter) {
			Fragment* frag = *aIter;
			bool active = (t._activeAtomFragments.find(frag) != t._activeAtomFragments.end());
			frag->trackState(active, _avResAlpha);
		}

		_foreach(_compFragments, cIter) {
			CompFragment* frag = *cIter;
			bool active = (t._activeComposites.find(frag) != t._activeComposites.end());
			frag->trackState(active, _avResAlpha);
		}
	}*/

    // probabilistically create new
    if (! _freezeDevelopment && _runPopAdjust && _probPostAgentParam) postCreateNew(t);


	/*_foreach(_atomFragments, aIter)
		(*aIter)->reset();

	_foreach(_compFragments, cIter)
		(*cIter)->reset();*/
}

void Harness::activateFrags(InstInfo& t) const
{
	vector<Fragment*>::const_iterator atomIter;
	vector<CompFragment*>::const_iterator pIter;
	tr1::unordered_map<int, std::tr1::unordered_set<CompFragment*> > depthMap;
	int maxDepth = 0;

	// find activation of atoms
	_foreach(_atomFragments, atomIter) {
		Fragment* thisFrag = *atomIter;
		//if (thisFrag->droppedOut()) continue;
		bool match = thisFrag->findActivation(t);

		if (match) {
			t._activeFragments.insert(thisFrag);
			t._activeAtomFragments.insert(thisFrag);
		}
		if (_testMethod == propAll || ((_testMethod == propActive) && match)) {
			const vector<CompFragment*>& pars = thisFrag->getParents();
			for (int i=0; i<pars.size(); ++i) {
				CompFragment* parent = pars[i];
				depthMap[parent->_linkDepth].insert(parent);
				maxDepth = _max(maxDepth, parent->_linkDepth);
			}
		}
	}

	// iterate over depths and activate comps
	int depth=1;
	std::tr1::unordered_set<CompFragment*>::iterator cIter;
	while(depth <= maxDepth) {
		std::tr1::unordered_set<CompFragment*>& comps = depthMap[depth];
		_foreach(comps, cIter) {
			CompFragment* comp = *cIter;
			//if (comp->droppedOut()) continue;
			bool match = comp->findActivation(t);

			if (match && ! comp->_storedFrag) {
				t._activeFragments.insert(comp);
				t._activeComposites.insert(comp);
			}
			if (_testMethod == propAll || ((_testMethod == propActive) && match)) {
				const vector<CompFragment*>& pars = comp->getParents();
				_foreach(pars, pIter) {
					// comp will be at higher depth, add to depth map and it will be accessed on next step
					CompFragment* parent = *pIter;
					depthMap[parent->_linkDepth].insert(parent);
					maxDepth = _max(maxDepth, parent->_linkDepth);
				}
			}
		}
		depth++;
	}
}

void Harness::maintainFragments()
{
	if (_freezeDevelopment || ! _runPopAdjust) return;

	if (_fragPopMethod == assocNorm) {
		// maintain frags according to sum of values (weights) used by softmax
		//if (_removalIterations && _gamesPlayed % _removalIterations == 0) {
			findAssocNorms();

			// sort fragments on assocNorm value
			sort(_atomFragments.begin(), _atomFragments.end(), Fragment::assocNormGThan);
			sort(_compFragments.begin(), _compFragments.end(), Fragment::assocNormGThan);

			// limit number of atomic fragments
			trimFragments();

			// limit number of composite fragments.  assume sorted
			trimComposites();
		//}
	} else if (_fragPopMethod == assocWtSum) {
		findAssocWtSums();

		// sort fragments on assocNorm value
		sort(_atomFragments.begin(), _atomFragments.end(), Fragment::assocNormGThan);
		sort(_compFragments.begin(), _compFragments.end(), Fragment::assocNormGThan);

		// limit number of atomic fragments
		trimFragments();

		// limit number of composite fragments.  assume sorted
		trimComposites();
	} else {
		logout << "population maintenance method " << _fragPopMethod << " not implemented" << endl;
		writeBufError();
	}

	// if using sparse output assocs, trim to size
    if (_sparseAssocLimit) { // && _removalIterations && _gamesPlayed % _removalIterations == 0) {
    	// collect all assoc values
    	vector<double> allVals;

		AssocMap::const_iterator mIter;
    	if (_activationSet == actActiveCompsAtoms || _activationSet == actAllCompsAtoms) {
    		// record atoms
    		vector<Fragment*>::iterator aIter;
    		_foreach(_atomFragments, aIter) {
    			const AssocMap& aMap = (*aIter)->getAssociationSet();
    			_foreach(aMap, mIter) allVals.push_back(mIter->second._val);
    		}
    	}
    	// record comp values
    	vector<CompFragment*>::iterator cIter;
		_foreach(_compFragments, cIter) {
			const AssocMap& aMap = (*cIter)->getAssociationSet();
			_foreach(aMap, mIter) allVals.push_back(mIter->second._val);
		}

		_numAssocs = allVals.size();
		if (_numAssocs > _sparseAssocLimit) {
			vector<double>::iterator cutoffEl = allVals.begin() + (allVals.size() - _sparseAssocLimit);
			std::nth_element(allVals.begin(), cutoffEl, allVals.end());

			double cutoff = *cutoffEl;

			// trim each feature
	    	if (_activationSet == actActiveCompsAtoms || _activationSet == actAllCompsAtoms) {
	    		// record atoms
	    		vector<Fragment*>::iterator aIter;
	    		_foreach(_atomFragments, aIter)
	    			_numAssocs -= (*aIter)->trimAssocs(cutoff);
	    	}
	    	vector<CompFragment*>::iterator cIter;
    		_foreach(_compFragments, cIter)
    			_numAssocs -= (*cIter)->trimAssocs(cutoff);
		}
    }
}

void Harness::findAssocNorms()
{
	vector<Fragment*>::iterator aIter;
    _foreach(_atomFragments, aIter) (*aIter)->findAssocNorm();

    vector<CompFragment*>::iterator cIter;
    _foreach(_compFragments, cIter) (*cIter)->findAssocNorm();

    // pass values down to make sure children given same value as parents
    _foreach(_compFragments, cIter) {
    	CompFragment* thisComp = *cIter;
    	double aSum = thisComp->getAssocNorm();
    	set<Fragment*> children;
    	thisComp->getChildSet(children);
    	set<Fragment*>::iterator chIter;
    	_foreach(children, chIter) (*chIter)->updateAssocNorm(aSum);
    }
}

void Harness::findAssocWtSums()
{
	vector<Fragment*>::iterator aIter;
    _foreach(_atomFragments, aIter) (*aIter)->findAssocNorm();

    vector<CompFragment*>::iterator cIter;
    _foreach(_compFragments, cIter) (*cIter)->findAssocNorm();

    // pass values down to make sure children given same value as parents
    _foreach(_compFragments, cIter) {
    	CompFragment* thisComp = *cIter;
    	double aSum = thisComp->getAssocNorm();
    	set<Fragment*> children;
    	thisComp->getChildSet(children);
    	set<Fragment*>::iterator chIter;
    	_foreach(children, chIter) (*chIter)->addAssocNorm(aSum);
    }
}

void Harness::display(ostream& output) const
{
	output << "avres " << _averageResult
		<< " atoms " << _numAtoms << "(" << _stats._atomsCreated << ")"
		<< " comps " << _numComposites << "(" << _stats._compsCreated << ")"
		<< " lactive a:" << _stats._lastActiveAtoms << ",c:"<< _stats._lastActiveComps
		<< " assocs:" << _numAssocs;

}

void Harness::displayState(ostream& output) const
{

}

void Harness::clearPopulation() {
	//< removes all fragments and instances

	// release all fragments
	int atoms = _maxAtomFragments;
	int comps = _maxComposites;
	_maxAtomFragments = _maxComposites = 0;
	trimFragments();
	trimComposites();
	_maxAtomFragments = atoms;
	_maxComposites = comps;
}


bool Harness::addElement(const string& elstring)
{
	// parse string and add given element
	warn("Harness::addElement not implemented");
	return false;
}


// for a given state and present fragments activated, build new
// fragments
void Harness::createAtomFragments(InstInfo& t, vector<Fragment*>* newFrags) const
{
	warn("Harness::createAtomFragments() not implemented\n");

}

void Harness::createCompFragments(InstInfo& t, vector<Fragment*>* newFrags) const
{
	warn("Harness::createCompFragments() not implemented\n");
}

float Harness::probOkCompFragments(int numFragments) {
	return exp(-_max(numFragments-1,0)); // condition to stop adding elements
}





CompFragment* Harness::makeComposite(const vector<Fragment*>& chosenFragments) {
	return new CompFragment(chosenFragments, this);
}
CompFragment* Harness::makeComposite(const tr1::unordered_map<int,Fragment*>& chosenFragMap) {
	return new CompFragment(chosenFragMap, this);
}

bool Harness::addFragment(Fragment* newFragment)
{
	//trace("adding fragment %p\n", newFragment);
	_Assert(! _freezeDevelopment, ("error adding fragment- frozen harness\n"));

	if (_insertEqCheck & insertCheckAtoms) {
		// check for equality with all existing frags
		vector<Fragment*>::iterator aIter;
		_foreach(_atomFragments, aIter)
			if (*newFragment == **aIter) return false;
	}

	++_numAtoms;
	++_stats._atomsCreated; // assume every one created will pass here, and only once
	_atomFragments.push_back(newFragment);
	//_atomFragVector.push_back(newFragment);

	// record any existing assocs
	_numAssocs += newFragment->getAssociationSet().size();

	if (_sparseAssocLimit) {
		// add assoc for new frag
		const vector<const Associating*>& vec = _worldPtr->_allClasses;
		if (! vec.empty()) {
			const Associating* cls = vec[_RandOf(vec.size())];
			if (newFragment->addAssoc(cls)) _numAssocs++;
		}
	}

	if (_showProcessing) {
		logout << "added new fragment " << newFragment->_fragID
				<< ", ";
		newFragment->display(logout);
		logout << endl;
		writeBuf();
	}
	_idMap[newFragment->_fragID] = newFragment;
	return true;
}

bool Harness::addComposite(CompFragment* newCompFragment)
{
	if (_insertEqCheck & insertCheckComps) {
		// check parents of child frags, to see if matching exists
		set<Fragment*> children;
		set<CompFragment*> parents;
		newCompFragment->getChildSet(children);
		set<Fragment*>::iterator iter;
		_foreach(children, iter) {
			if (*iter == newCompFragment) continue;
			const vector<CompFragment*>& pars = (*iter)->getParents();
			parents.insert(pars.begin(), pars.end());
		}
		set<CompFragment*>::iterator pIter;
		_foreach(parents, pIter) {
			if (*pIter == newCompFragment) continue; // ignore self
			if (**pIter == *newCompFragment) return false; // identical existing frag
		}
	}

	//trace("adding composite %p\n", newCompFragment);
	_Assert(! _freezeDevelopment, ("error adding composite- frozen harness\n"));
	++_numComposites;
	++_stats._compsCreated; // assume every one created will pass here, and only once
	_compFragments.push_back(newCompFragment);
	//_compFragVector.push_back(newCompFragment);

	// record any existing assocs
	_numAssocs += newCompFragment->getAssociationSet().size();

	if (_sparseAssocLimit) {
		// add assoc for new frag
		const vector<const Associating*>& vec = _worldPtr->_allClasses;
		if (! vec.empty()) {
			const Associating* cls = vec[_RandOf(vec.size())];
			if (newCompFragment->addAssoc(cls)) _numAssocs++;
		}
	}

	if (_showProcessing) {
		logout << "added new composite " << newCompFragment->_fragID <<
				", " << newCompFragment->displayStr() << endl;
		writeBuf();
	}
	_idMap[newCompFragment->_fragID] = newCompFragment;

	return true;
}

bool Harness::removeFragment(Fragment* rmFragment) {
	// find position of fragment in list
	// find position of element in fragments list
	vector<Fragment*>::iterator position = find(_atomFragments.begin(), _atomFragments.end(), rmFragment);
	if (position == _atomFragments.end()) return false;

	// remove all linked elements
	rmFragment->removeParents();

	// remove assocs from count
	_numAssocs -= rmFragment->getAssociationSet().size();
	_idMap.erase(rmFragment->_fragID);
	if (_showProcessing) {
		logout << "removing fragment " << rmFragment->_fragID << endl;
		writeBuf();
	}
	delete rmFragment;

	_atomFragments.erase(position);

	return true;
}

void Harness::trimFragments()
{
	_Assert(! _freezeDevelopment, ("error removing fragment- frozen harness\n"));

	if (_numAtoms <= _maxAtomFragments) return;

	// remove each of marked fragments
	for (vector<Fragment*>::iterator i = _atomFragments.begin() + _maxAtomFragments; i != _atomFragments.end(); i++) {
		Fragment* rmFragment = *i;

		// remove all linked elements
		rmFragment->removeParents();
		_numAssocs -= rmFragment->getAssociationSet().size();
		_idMap.erase(rmFragment->_fragID);

		if (_showProcessing) {
			logout << "removing fragment " << rmFragment->_fragID << endl;
			writeBuf();
		}
		delete rmFragment;
	}

	_numAtoms = _maxAtomFragments;

	_atomFragments.resize(_maxAtomFragments);
}

bool Harness::removeComposite(CompFragment* rmCompFragment)
{
	// TODO: handle rules when composites they own are removed

	vector<CompFragment*>::iterator position = find(_compFragments.begin(), _compFragments.end(), rmCompFragment);
	if (position == _compFragments.end()) {
		logout << "could not find comp " << rmCompFragment << " to remove" << endl;
		writeBufWarn();
		return false;
	}

	// remove all linked elements
	rmCompFragment->removeParents();
	_idMap.erase(rmCompFragment->_fragID);

	_numAssocs -= rmCompFragment->getAssociationSet().size();

	if (_showProcessing) {
		logout << "removing comp: " << rmCompFragment->_fragID << " " <<
			rmCompFragment->displayStr() << endl;
		writeBuf();
	}
	delete rmCompFragment;

	_compFragments.erase(position);
	return true;
}


void Harness::trimComposites()
{
	_Assert(! _freezeDevelopment, ("error removing fragment- frozen harness\n"));

	if (_numComposites <= _maxComposites) return;

	// remove each of marked fragments
	for (int i = _maxComposites; i < _compFragments.size(); i++) {
		CompFragment* rmCompFragment = _compFragments[i];

		// remove all linked elements
		rmCompFragment->removeParents();
		_numAssocs -= rmCompFragment->getAssociationSet().size();
		_idMap.erase(rmCompFragment->_fragID);

		if (_showProcessing) {
			logout << "removing comp: " << rmCompFragment->_fragID << " " <<
				rmCompFragment->displayStr() << endl;
			writeBuf();
		}
		delete rmCompFragment;
	}

	_numComposites = _maxComposites;

	_compFragments.resize(_maxComposites);
}

const Associating* Harness::findOutputActivations(InstInfo& t) const {
    // record sum of activations for each class

	// look at associations from activated fragments, determine
    // highest predicted classification.  only use comp frags
    const Associating* bestClass = NULL;

    vector<Fragment*> iList;
    t._activationFrags.clear();
    if (_activationSet == actAllComps || _activationSet == actAllCompsAtoms) {
    	vector<CompFragment*>::const_iterator cIter;
    	_foreach(_compFragments, cIter) {
    		CompFragment* thisFrag = *cIter;
    		if (_minDepth && thisFrag->_linkDepth < _minDepth) continue;
    		//if (thisFrag->droppedOut()) continue;
    		t._activationFrags.push_back(thisFrag);
    	}
    }
    if (_activationSet == actAllCompsAtoms) {
    	vector<Fragment*>::const_iterator aIter;
    	_foreach(_atomFragments, aIter) {
    		Fragment* thisFrag = *aIter;
    		if (_minDepth) continue;
    		//if (thisFrag->droppedOut()) continue;
    		t._activationFrags.push_back(thisFrag);
    	}
    }
    if (_activationSet == actActive) {
    	std::tr1::unordered_set<CompFragment*>::iterator sIter;
    	_foreach(t._activeComposites, sIter) {
    		CompFragment* thisFrag = *sIter;
    		if (_minDepth && thisFrag->_linkDepth < _minDepth) continue;
    		//if (thisFrag->droppedOut()) continue;
    		t._activationFrags.push_back(thisFrag);
    	}
    }
    if (_activationSet == actActiveCompsAtoms) {
    	std::tr1::unordered_set<Fragment*>::iterator sIter;
    	_foreach(t._activeFragments, sIter) {
    		Fragment* thisFrag = *sIter;
    		if (_minDepth && thisFrag->_linkDepth < _minDepth) continue;
    		//if (thisFrag->droppedOut()) continue;
    		t._activationFrags.push_back(thisFrag);
    	}
    }

	// assume list defined

	// pass call to thread handlers

    procOutputActivations(t, t._activationFrags.begin(), t._activationFrags.end());


	// finalise from output sums

	// find sum of activations
	double activSum = 0.0;
	if (_outActivationMethod == selSoftMax) {
		ClassMap::iterator fIter;
		_foreach(t._classActivations, fIter) {
			const Associating* thisClass = fIter->first;
			double thisAct = fIter->second;
			tr1::unordered_map<const Associating*, NeurParam>::const_iterator cIter
				= _classBiases.find(thisClass);
			double thisBias = cIter->second._val;

			//double thisBias = _classBiases[thisClass]._val;

			if (thisAct > MAXEXP) {
				logout << "potential overflow, exp " << thisAct << endl;
				writeBufWarn();
				thisAct = MAXEXP;
			}
			activSum += exp(thisAct + thisBias);
		}
	}
	// set action according to sum
	if (_outActivationMethod == selSum || _outActivationMethod == selSoftMax) {
    	double bestActSum = 0.0;
		ClassMap::iterator fIter;
		_foreach(t._classActivations, fIter) {
			const Associating* thisClass = fIter->first;
			double thisAct = fIter->second;

			double thisBias = _classBiases.find(thisClass)->second._val;
			//double thisBias = _classBiases[thisClass]._val;
			if (_outActivationMethod == selSoftMax) {
				// normalise all values against sum
				if (activSum == 0.0) thisAct = 0.0;
				else thisAct = exp(thisAct + thisBias) / activSum;
			} else {
				// add class bias to activation and apply transfer fn
				thisAct = NeurFn(thisAct + thisBias);
			}

			t._outputs[thisClass] = thisAct;

			// choose highest
			if (bestClass == NULL || thisAct > bestActSum) {
				bestClass = thisClass;
				bestActSum = thisAct;
			}
		}

		// if binary classification (one class), check output level
		if (_outActivationMethod == selSum && _worldPtr->getBinaryClass()) {
			// assume single class association being used, check if value is above mean
			pair<Associating*, Associating*> binClasses = _worldPtr->getBinaryClasses();

			double thresh = Params::_NEUTRVAL;

			if (t._outputs[binClasses.second] < thresh) bestClass = binClasses.first;
			else bestClass = binClasses.second;
		}
    }
	return bestClass;
}


void Harness::procOutputActivations(InstInfo& t, vector<Fragment*>::iterator iter, vector<Fragment*>::iterator last) const {
	// get a frag from the queue.  assume first and last defined?

	ClassMap outputs;

	// pass activations from each chosen composite to classes
	while (iter != last) {
		Fragment* thisFrag = *iter;

		// assume all associations are with classes
		const AssocMap& asscSet = thisFrag->getAssociationSet();
		//logout << "frag " << thisFrag << " ";

		AssocMap::const_iterator sIter;
		_foreach(asscSet, sIter) {
			// record class activations (no bias)
			outputs[sIter->first] += sIter->second._val * t._fragDetails[thisFrag]._activation;
//			logout << "w " << sIter->second._val << " a " << t._fragDetails[thisFrag]._activation << " o " << outputs[sIter->first] << endl;
		}

		iter++;
	}
	ClassMap::iterator aIter;
	do {
		// thread lock
		_foreach(outputs, aIter) {
			t._classActivations[aIter->first] += aIter->second;
//			logout << "ca " << t._classActivations[aIter->first] << endl;
		}
	} while(0);
//	writeBuf();
}

void Harness::clearAllAssocs() {
	//< reset assoc values of all frags
	vector<Fragment*>::iterator aIter;
	_foreach(_atomFragments, aIter)
		(*aIter)->clearAllAssociations();

	vector<CompFragment*>::iterator cIter;
	_foreach(_compFragments, cIter)
		(*cIter)->clearAllAssociations();

	// clear all bias values.
	_classBiases.clear();
}

double Harness::adjustFragOutput(InstInfo& t, Fragment* thisFrag) const {
	double deltaSum = 0.0;
	double thisActivation = t._fragDetails[thisFrag]._activation;
	AssocMap& thisAssocs = thisFrag->getModAssociationSet();
	AssocMap::iterator aIter;
	ClassMap::iterator iter;

	// iterate over current assoc links if using limits
	if (_sparseAssocLimit) {
		_foreach(thisAssocs, aIter) {
			NeurParam& param = aIter->second;

			// find sum of deltas.  (assume class value from map is valid)
			deltaSum += t._classDeltas[aIter->first] * param._val;

			// adjust weight to class (output) node
			double grad = t._classDeltas[aIter->first] * thisActivation;
			recordChange(thisFrag, grad, &param);
		}
	} else {
		// create new output links if not found
		_foreach(t._outputs, iter) {
			const Associating* thisClass = iter->first;

			double grad = t._classDeltas[thisClass] * thisActivation;

			// get parameter, will be set to 0.0 if not found
			if ((aIter = thisAssocs.find(thisClass)) == thisAssocs.end()) {
				// record new weight
				t._fragDetails[thisFrag]._newAssocClasses.push_back(thisClass);
				t._fragDetails[thisFrag]._newAssocVals.push_back(grad);
			} else {
				NeurParam& param = aIter->second;  //thisFrag->getAssocParam(thisClass);

				// find sum of deltas
				deltaSum += t._classDeltas[thisClass] * param._val;

				// adjust weight to class (output) node
				recordChange(thisFrag, grad, &param);
			}
		}
	}
	return deltaSum;
}

void Harness::runBackprop(InstInfo& t) const {
    // clear frag delta values
	tr1::unordered_map<Fragment*, double> fragDeltaSum;

	tr1::unordered_map<int, std::tr1::unordered_set<CompFragment*> > depthMap;
	std::tr1::unordered_set<Fragment*> atomFrags;
	int maxDepth = 0;

	// find delta values for each activation frag, from output nodes
	// update frag->output weights
	vector<Fragment*>::iterator iter;
    _foreach(t._activationFrags, iter) {
    	Fragment* thisFrag = *iter;

		// adjust assoc for each class
		double deltaSum = adjustFragOutput(t, thisFrag);


		// record deltas for each (don't include g'(x) product here, do once later)
		//double thisDelta = deltaSum * NeurDelta(thisActivation);
		// (parallel requires lock or split & merge)
		fragDeltaSum[thisFrag] = deltaSum;

		// record in list of frags to update
		// (parallel requires lock or split & merge)
		if (thisFrag->isComposite()) {
			if (deltaSum != 0.0) {
				depthMap[thisFrag->_linkDepth].insert(static_cast<CompFragment*>(thisFrag));
			}
		} else {
			atomFrags.insert(thisFrag);
		}
		maxDepth = _max(maxDepth, thisFrag->_linkDepth);
    }

    // update network weights
    if (_backpropMethod & bpComps) {
		std::tr1::unordered_set<CompFragment*>::iterator sIter;
		for (int d = maxDepth; d > 0; d--) {
			// (following loop can be split & run parallel, with locks or merged updates to fragDeltaSum)
			_foreach(depthMap[d], sIter) {
				CompFragment* examineFrag = *sIter;

				// include derivative of activation to give delta values, for use in
				// updating child elements
				double thisActivation = t._fragDetails[examineFrag]._activation;
				double thisDelta  = fragDeltaSum[examineFrag] * NeurDelta(thisActivation);
				//if (fabs(thisDelta) < MINDELTA) continue;
				if (thisDelta == 0.0) continue; // timid check

				// adjust bias value for this comp frag
				double biasGrad = thisDelta;
				NeurParam& biasParam = examineFrag->biasParam();
				recordChange(examineFrag, biasGrad, &biasParam);

				// update child weights
				const tr1::unordered_map<int, Fragment*>& childFrags
					= examineFrag->getComponents();
				tr1::unordered_map<int, Fragment*>::const_iterator cIter;
				_foreach(childFrags, cIter) {
					Fragment* thisChild = cIter->second;
					//if (thisChild->droppedOut()) continue;

					if (thisChild->isComposite())
						depthMap[d-1].insert(static_cast<CompFragment*>(thisChild));
					else
						atomFrags.insert(thisChild);

					// adjust delta value of child according to child weight
					NeurParam& chWeight = examineFrag->getChildWeightParam(cIter->first);
					// (parallel needs lock or split & merge updates:)
					fragDeltaSum[thisChild] += thisDelta * chWeight._val;

					// then update child weight
					double childGrad = t._fragDetails[thisChild]._activation * thisDelta;
					recordChange(thisChild, childGrad, &chWeight);
				} // each child
			} // each comp at depth
		} // each depth
    } // run bp on comp network weights
	else if (_backpropMethod & bpPosition) {
		// pass to older position based call
		list<CompFragment*> fragOrder;
		tr1::unordered_map<int, std::tr1::unordered_set<CompFragment*> >::iterator dIter;
		_foreach(depthMap, dIter) fragOrder.insert(fragOrder.end(), dIter->second.begin(), dIter->second.end());
		backpropagatePosition(fragOrder);
	}

    // run bp on atom frags, to update connections with observations
    // (can be split & run parallel)
	if (_backpropMethod & bpAtoms) {
		std::tr1::unordered_set<Fragment*>::iterator aIter;
		_foreach(atomFrags, aIter) {
			Fragment* atom = *aIter;
			double grad = fragDeltaSum[atom] * NeurDelta(t._fragDetails[atom]._activation);
			//if (fabs(thisDelta) < MINDELTA) continue;

			atom->doBackprop(t, grad);
		}
	}
}


bool Harness::postCreateNew(InstInfo& t) const
{
	//< create method after result given

	// create based on average result and current result
	// end of turn (post action).  if in training mode, find
	// correct class from world and allocate rewards accordingly
	// to agents stored in action sets

	// get class object for associating
	float result = (t._currentClass == _chosenClass ? 1.0 : 0.0);

	//#define MIDVAL ((OUTHTARGET + OUTLTARGET) / 2.0)
	double beta = Params::_pinstance->_reproductionBeta;

	//float probNew = exp(beta * (-(result - _averageResult) - 1.0)) * _probPostAgentParam;
	//float probNew = _probPostAgentParam * 1.0 / (1.0 + exp(beta * (result - _averageResult)));
	float probNew = _probPostAgentParam * 1.0 / (1.0 + exp(beta * (result - 0.5)));

	if (probNew < 0.0) probNew = 0.0;
	if (probNew > 1.0) probNew = 1.0;
	//_Assert(probNew >= 0.0 && probNew <= 1.0, ("Invalid probability\n"));
	if (! _freezeDevelopment && _pRand(probNew)) {
		vector<Fragment*> newFrags;

		// add new agent from world state instead of selecting existing
	    if (_pRand(_atomRatio)) createAtomFragments(t, &newFrags);
	    createCompFragments(t, &newFrags);

		// perform one training step using the new frags
		vector<Fragment*>::iterator iter;
		_foreach(newFrags, iter) {
			// assume activated, adjust weights to outputs
			adjustFragOutput(t, *iter);
		}
	    return true;
	}

	return false;
}

void Harness::outputWeights(InstInfo& t, ostream& os) const
{
	// display weights between composites and classes

	// first make ordered set of classes.
	// just sort on class pointer? hopefully consistent
	ClassMap::const_iterator iter;
	list<const Associating*> cList;
	_foreach(t._outputs, iter) {
		cList.push_back(iter->first);
	}
	cList.sort(); // sorts on pointer
	list<const Associating*>::iterator lIter;

	vector<CompFragment*>::const_iterator cIter;
	_foreach(_compFragments, cIter) {
		CompFragment* thisComp = *cIter;
		const AssocMap& thisAssocs = thisComp->getAssociationSet();
		AssocMap::const_iterator fIter;

		// output this comp ID
		os << thisComp->_fragID;
		_foreach(cList, lIter) {
			// output weight between comp and class
			fIter = thisAssocs.find(*lIter);
			if (fIter == thisAssocs.end())
				os << " " << 0.0;
			else os << " " << fIter->second._val;
		}
		os << endl;
	}
}

void Harness::backpropagatePosition(list<CompFragment*>& fragOrder) const
{
	// need higher level implementation
	warn("Harness::backpropagatePosition not implemented");
}

unsigned int Harness::getFragID()
{
	boost::mutex::scoped_lock lock(_mutex);
	return _maxFragID++;
}

void Harness::incrAssocs() // locks
{
	boost::mutex::scoped_lock lock(_mutex);
	_numAssocs++;
}

void Harness::writeModel(ostream& out) const
{
	// output bias weights
	//out << "biases";
	out << _classBiases.size();
    tr1::unordered_map<const Associating*, NeurParam>::const_iterator bIter;
    _foreach(_classBiases, bIter) {
    	out << " ";
    	bIter->first->display(out);
    	out << " " << bIter->second._val << " " << bIter->second._mom;
    }
    out << endl;

    // output atom frags
    out << _numAtoms << endl;
    vector<Fragment*>::const_iterator aIter;
    _foreach(_atomFragments, aIter) {
    	(*aIter)->write(out);
    	out << endl;
    }

    // output comps in depth order
    out << _numComposites << endl;
    tr1::unordered_map<int, vector<CompFragment*> > depthMap;
    int maxDepth=0;
    vector<CompFragment*>::const_iterator cIter;
    _foreach(_compFragments, cIter) {
    	depthMap[(*cIter)->_linkDepth].push_back(*cIter);
    	maxDepth = _max(maxDepth, (*cIter)->_linkDepth);
    }
    for (int d=0; d<=maxDepth; ++d) {
    	_foreach(depthMap[d], cIter) {
			(*cIter)->write(out);
			out << endl;
    	}
    }
}
void Harness::readModel(istream& in)
{
	// read bias weights
	_classBiases.clear();
	int n;
	in >> n;
	logout << "reading " << n << " class bias weights" << endl; writeBuf();
	for (int i=0; i<n; ++i) {
		int classID;
		double wtval, wtmom;
		in >> classID >> wtval >> wtmom;
		_classBiases[_worldPtr->getClassObj(classID)]._val = wtval;
		_classBiases[_worldPtr->getClassObj(classID)]._mom = wtmom;
	}

	// read atoms
	int readFrags;
	in >> readFrags;
	logout << "reading " << readFrags << " atom fragments" << endl; writeBuf();
	for (int i=0; i<readFrags; ++i) {
		Fragment* atom = readAtom(in);
		if (atom) {
			if (addFragment(atom)) {
				_maxFragID = _max(_maxFragID, atom->_fragID);
			} else {
				atom->removeParents();
				delete atom;
			}
		}
	}

	// read comps
	in >> readFrags;
	logout << "reading " << readFrags << " composite fragments" << endl; writeBuf();
	for (int i=0; i< readFrags; ++i) {
		CompFragment* comp = readComp(in);
		if (comp) {
			if (addComposite(comp)) {
				_maxFragID = _max(_maxFragID, comp->_fragID);
			} else {
				comp->removeParents();
				delete comp;
			}
		}
	}
	logout << "done" << endl; writeBuf();

	_maxFragID++;
}

Fragment* Harness::readAtom(istream&){
	return NULL;
}
CompFragment* Harness::readComp(istream&) {
	return NULL;
}


void Harness::applyChanges(vector<InstInfo*>& batch, int batchSize)
{
	_Assert(_numAtoms == _atomFragments.size(), ("atoms doesn't match numAtoms\n"));
	_Assert(_numComposites == _compFragments.size(), ("comps doesn't match numAtoms\n"));
	int totFrags = _numAtoms + _numComposites;
	int procAtoms = totFrags;
	if (Params::_pinstance->_nThreads) procAtoms = totFrags / Params::_pinstance->_nThreads;
	int lowBound = 0, highBound = procAtoms;
	for (int n=0; n< batchSize; ++n) {
		InstInfo& t = *batch[n];

		if (highBound > lowBound) {
			// create task to update a given range of fragments
			Task upd;
			upd._job = jobApplyUpdates;
			upd._atoms = &_atomFragments;
			upd._comps = &_compFragments;
			upd._low = lowBound;
			upd._high = highBound;
			upd._batch = &batch;
			upd._batchSize = batchSize;
			upd._curInst = &t;
			if (Params::_pinstance->_nThreads) {
				boost::mutex::scoped_lock lock(JobThread::queue_mut);
				JobThread::job_queue.push_back(upd);
				JobThread::queue_change.notify_one();
			} else {
				// running without threads
				t.processFrags(upd);
			}
		}
		lowBound = highBound;
		highBound = _min(totFrags, highBound + procAtoms);
	}

	// wait for threads
	if (Params::_pinstance->_nThreads) {
		JobThread::waitQueueFinished();
		JobThread::waitAllIdle();
	}
	tr1::unordered_map<NeurParam*, vector<double> > bChanges;
	for (int n=0; n< batchSize; ++n) {
		InstInfo& t = *batch[n];

		// apply output
		tr1::unordered_map<const Associating*, double>::iterator bIter;
		_foreach(t._classBiasMods, bIter) {
			bChanges[&_classBiases[bIter->first]].push_back(bIter->second);
		}

		// add recorded fragments
		// new atoms
		vector<Fragment*>::iterator aIter;
		_foreach(t._newAtoms, aIter) {
			Fragment* newFrag = *aIter;

			//logout << "recording atom " << newFrag << endl;
			//writeBuf();
			if (! addFragment(newFrag)) {
				newFrag->removeParents();
				delete newFrag;
			}
		}

		vector<CompFragment*>::iterator cIter;
		_foreach(t._newComps, cIter) {
			CompFragment* newComposite = *cIter;
			//logout << "recording comp " << newComposite << endl;
			//newComposite->display(logout);
			//writeBuf();
			if (addComposite(newComposite)) {
				// was created without parent links from children
				// (to avoid thread conflict)
				newComposite->linkChildren();
			} else {
				newComposite->removeParents();
				delete newComposite;
				newComposite = NULL;
			}
		}
	}

	// apply changes to biases as average gradient
	tr1::unordered_map<NeurParam*, vector<double> >::iterator bIter;
	vector<double>::iterator lIter;
	_foreach(bChanges, bIter) {
		NeurParam& param = *bIter->first;

		double avgGrad = 0.0;
		double count = 1.0;
		_foreach(bIter->second, lIter) {
			avgGrad += (*lIter - avgGrad) / count;
			count += 1.0;
		}
		updateWts(avgGrad, param);
	}
}


FragStatCollector::FragStatCollector(const Harness& watchHarness)
 : _watchHarness(watchHarness)
{
    // read parameters

    _regInt(_dumpFragsInterval, IKEY_DUMPFRAGSIVL); //, 10000);
	_regInt(_fragInfoChannel, IKEY_FRAGINFOCHANNEL); //, 0);
	_nextDumpFrags = 0;
}

FragStatCollector::~FragStatCollector()
{

}

void FragStatCollector::doOutputStats(int iteration)
{

	// give output of component, categorising and transformation elements
	if (_dumpFragsInterval && (iteration >= _nextDumpFrags)) {
		if (_fragInfoChannel) {
	        logout << "== " << " iter " << iteration << endl;
			writeBuf(_fragInfoChannel, TRACE);
			//record(_fragInfoChannel, TRACE, "== %s iter %d\n", _name.c_str(), iteration);
			//logout << fragInfo();
			_watchHarness.writeModel(logout);
			writeBuf(_fragInfoChannel, TRACE);
	        //record(_fragInfoChannel, TRACE, fragInfo());
		}
		_nextDumpFrags += _dumpFragsInterval;
	}

}


string FragStatCollector::fragInfo()
{
	ostringstream oss;

	// record atoms
	oss << "--Atoms--" << endl;
	vector<Fragment*>::const_iterator atomIter;
	_foreach(_watchHarness._atomFragments, atomIter) {
		Fragment* thisAtom = *atomIter;
		oss << "atom: " << thisAtom << " ";
		thisAtom->display(oss);
		oss << endl;
	}

	// record composites
	oss << endl << "--Composites--" << endl;
	vector<CompFragment*>::const_iterator compIter;
	_foreach(_watchHarness._compFragments, compIter) {
		CompFragment* thisComp = *compIter;
		oss << "comp: " << thisComp << " ";
		thisComp->display(oss);
		oss << endl;
	}

	return oss.str();
}


} // namespace evagents
