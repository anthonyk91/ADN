/*
 * InstHarness.cpp
 *
 *  Created on: 26/05/2014
 *      Author: anthony
 */

#include "ADN.h"

namespace adn {

InstHarness::InstHarness() {
	_compInstFreePtr = _compInstanceStore.end();
	_atomInstFreePtr = _atomInstanceStore.end();


}

InstHarness::~InstHarness() {
}

void InstHarness::clearPopulation() {
	Harness::clearPopulation();
	//< removes all fragments and instances

	// delete all owned instances
	_foreach(_compInstanceStore, _compInstFreePtr) delete *_compInstFreePtr;
	_foreach(_atomInstanceStore, _atomInstFreePtr) delete *_atomInstFreePtr;
	_compInstanceStore.clear();
	_atomInstanceStore.clear();
	_compInstFreePtr = _compInstanceStore.end();
	_atomInstFreePtr = _atomInstanceStore.end();
}

void InstHarness::observeState()
{
    _instancesCreated = 0;
    _countInstances = 0;

    Harness::observeState();
}


CompInstance* InstHarness::makeCompInstance(const tr1::unordered_map<int,AsscInstance*>& substs, CompFragment& templateFrag) {
	// should be subclassed?

	// create just as CompInstance
	return new CompInstance(substs, templateFrag);
}
CompInstance* InstHarness::getCompInstance(const tr1::unordered_map<int,AsscInstance*>& substs, CompFragment& templateFrag, bool removeObj)
{
	//< create new object or redefine existing from store
	CompInstance* foundInstance = NULL;

	// look for free instance
	if (_compInstFreePtr != _compInstanceStore.end()) {
		foundInstance = *_compInstFreePtr;
		if (removeObj) _compInstFreePtr = _compInstanceStore.erase(_compInstFreePtr);
		else ++_compInstFreePtr;

		// define instance
		foundInstance->reset();
		foundInstance->setSubsts(substs);
		foundInstance->setTemplate(templateFrag);
		//trace("re-def comp instance\n");
	} else {
		// create new instance and put in store.
		// (create CompInstance object here? or leave to subclass)
		foundInstance = makeCompInstance(substs, templateFrag);
		_compInstanceStore.push_back(foundInstance);
		//trace("new comp instance\n");
	}

	return foundInstance;
}

void InstHarness::releaseCompInst(CompInstance* inst)
{
	// assume was removed from list, add to end

	// find in store and remove
	list<CompInstance*>::iterator sIter;
	bool found = false;
	_foreach(_compInstanceStore, sIter) {
		// if reach free item pointer, stop
		if (sIter == _compInstFreePtr) break;
		if (*sIter == inst) {
			_compInstanceStore.erase(sIter);
			found = true;
			break;
		}
	}
	_Assert(found, ("invalid- attempt to release comp instance, not found\n"));

	// put at end of list
	list<CompInstance*>::iterator insIter
		= _compInstanceStore.insert(_compInstanceStore.end(), inst);

	// check if free pointer is invalidated, if so set to inserted element
	if (_compInstFreePtr == _compInstanceStore.end())
		_compInstFreePtr = insIter;

	//logout << "returning instance " << inst << endl; writeBuf();
}

void InstHarness::display(ostream& output) const
{
	Harness::display(output);
	output << ",maxinst:" << _stats._maxInsts << "(" <<  _stats._maxInstsDefault << " d)"
			<< ",it:" << _stats._maxExaminedIterations
			<< "(" << _stats._maxIterWeights << " w),cinsts:" << _countInstances;

	_stats._maxInsts = 0;
	_stats._maxInstsDefault = 0;
	_stats._maxInstsExisting = 0;
	_stats._maxExaminedIterations = 0;
}

AsscInstance* InstHarness::makeFragInstance(Fragment& parentFragment) {
	//< create new, allow subclassing
	return new AsscInstance(parentFragment);
}

//< create new or redefine existing from store.  just returns the fragment
// object, needs to be defined elsewhere
AsscInstance* InstHarness::getFragInstance(Fragment& parent)
{
	//< create new or redefine existing from store
	AsscInstance* foundInstance = NULL;

	// look for free instance
	if (_atomInstFreePtr != _atomInstanceStore.end()) {
		foundInstance = *_atomInstFreePtr;
		foundInstance->reset();
		foundInstance->setTemplate(parent);
		++_atomInstFreePtr;
	} else {
		// create new instance and put in store
		foundInstance = makeFragInstance(parent);
		_atomInstanceStore.push_back(foundInstance);
		//trace("new atom instance\n");
	}

	if (! foundInstance) warn("error creating/finding atom instance\n");
	return foundInstance;
}

void InstHarness::clearAllInstances()
{
	//trace("clearing instances\n");

	// simply shift "free" pointers to beginning of list.  assume elements
	// will not attempt to be referred to
	_compInstFreePtr = _compInstanceStore.begin();
	_atomInstFreePtr = _atomInstanceStore.begin();
}


} /* namespace binaryrep */
