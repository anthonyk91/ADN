/*
 * InstHarness.h
 *
 *  Created on: 26/05/2014
 *      Author: anthony
 */

#ifndef INSTHARNESS_H_
#define INSTHARNESS_H_

namespace adn {

class InstHarness : public Harness {
public:
	InstHarness();
	virtual ~InstHarness();

	virtual void clearPopulation(); //< removes all fragments and instances

	// instance methods
	virtual CompInstance* makeCompInstance(const tr1::unordered_map<int,AsscInstance*>& substs, CompFragment& templateFrag);
    virtual CompInstance* getCompInstance(const tr1::unordered_map<int,AsscInstance*>& substs, CompFragment& templateFrag, bool removeObj = false); //< create new or redefine existing from store
    virtual void releaseCompInst(CompInstance* inst);
    virtual AsscInstance* makeFragInstance(Fragment& parentFragment); //< create new, allow subclassing
    AsscInstance* getFragInstance(Fragment& parentFragment); //< create new or redefine existing from store

	virtual void observeState();
	virtual void display(ostream& output) const;


    unsigned int _countInstances; // updated by fragments

protected:
	unsigned int _instancesCreated;

	// instances
	void clearAllInstances();
	void mergeFragments(Fragment* a, Fragment* b);
	void replaceRedundantFrag(CompFragment* comp);

    list<CompInstance*> _compInstanceStore;
    list<AsscInstance*> _atomInstanceStore;

    list<CompInstance*>::iterator _compInstFreePtr;
    list<AsscInstance*>::iterator _atomInstFreePtr;


};

} /* namespace binaryrep */

#endif /* INSTHARNESS_H_ */
