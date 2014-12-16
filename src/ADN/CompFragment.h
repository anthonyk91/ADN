#ifndef COMPFRAGMENT_H_
#define COMPFRAGMENT_H_

#include "Displaying.h"
#include <set>
#include <vector>
#include <tr1/unordered_map>

namespace adn {

class CompProperty;
class CompInstance;

typedef std::tr1::unordered_map<int,Fragment*> FragMap;
typedef std::tr1::unordered_map<int,AsscInstance*> InstMap;
typedef list<unsigned int> Traversal;

class CompFragment : public Fragment
{
public:
	CompFragment(const vector<Fragment*>& elements,	Harness* parent, float biasVal = 0.0, bool linkChildren = true);
	CompFragment(const FragMap& elements,Harness* parent, float biasVal = 0.0, bool linkChildren = true);
	CompFragment(const CompFragment& f); // copy const, create duplicate properties
	virtual ~CompFragment();
   	CompFragment& operator=(const CompFragment& f);
	virtual bool operator==(const Fragment& f); // check if matches
   	virtual bool operator==(const CompFragment& frag);

	virtual void addProperty(CompProperty* prop); // claims ownership of property

	unsigned int numElements();
	
	const FragMap& getComponents() const;
	Fragment* getComponent(unsigned int key) const;
	
	virtual void getChildSet(set<Fragment*>& recordSet);
	virtual void getChildSet(set<const Fragment*>& recordSet) const;
	virtual void getChildList(list<Fragment*>& recordList);
	virtual void getChildList(list<const Fragment*>& recordList) const;
	virtual void getAtomElements(list<Fragment*>& atomSet) const;
	virtual unsigned int replaceChild(Fragment* a, Fragment* b); //< replace all occurances of B with A.  first is the one that will be maintained 
	unsigned int removeChild(Fragment* frag);
	virtual int addChild(Fragment* a); //< returns new key
	void linkChildren();

	virtual void display(ostream&) const;
	virtual void displayRecursive(ostream&) const;
	virtual bool redundantFrag() const;

	virtual bool isComposite() const;
	virtual void addElement(Fragment* el);
	virtual void removeElement(int key, bool removeProperties); //Fragment* el);
	virtual void replaceAtomRefs(Fragment* prevAtom, Fragment* newAtom);

    const set<CompProperty*>& getCompProperties() const;
    virtual CompInstance* createInstance(const tr1::unordered_map<int,AsscInstance*>& substs, bool removeObj = false);

    virtual double getChildWeight(int idx);
	virtual NeurParam& getChildWeightParam(int key);

    virtual void write(ostream&) const;
    virtual void read(istream&);
	bool _storedFrag;
protected:
    void releaseProperties();
    void duplicateProperties(const set<CompProperty*>& properties);
    void releaseChildFragments();
    void copyChildFragments(const FragMap& newEls);
    
    FragMap _childMap;
    int _nextKey;

    set<CompProperty*> _compProperties;
	tr1::unordered_map<int, NeurParam> _childWeights;

};

class CompProperty : public Displaying
{
public:
	CompProperty(CompFragment* parent)
		: _parent(parent) {}
	virtual ~CompProperty();

	virtual bool operator==(const CompProperty&) = 0; // overload to test identical properties.  note overload will have to test against CompProperty& parameter 
	virtual bool propertyMatches(const InstMap& substs) = 0;
    virtual bool propertyMatches(const CompInstance& inst);
    virtual bool propertyMatches(const CompFragment& frag, CompInstance** foundInst = NULL); // check all instances for match
    
	virtual bool getComponentsReferenced(set<Fragment*>& foundComponents) = 0;
    virtual bool referencesComponent(Fragment* component);
    
    virtual CompProperty* clone() = 0;
	virtual void display(ostream&) const = 0;
	void setNewParent(CompFragment&);
	
	virtual bool matchesKey(int i) = 0;
	virtual bool matchesTraversal(Traversal t, Traversal prefixTrav) = 0;
	CompFragment* getParent() const;

protected:
    CompFragment* _parent;
	float _weight;
};

class CompInstance : public AsscInstance, public Observer<CompInstance,AsscInstance>
{
public:
	CompInstance(const InstMap& substs, Fragment& parentFrag);
	virtual ~CompInstance();

	virtual bool operator==(const CompInstance& compareInsts) const;
	bool operator<(const CompInstance& compareInsts) const;
	bool matchesSubsts(const InstMap& substs) const;
	void setSubsts(const InstMap& substs);
	
	const InstMap& getComponentInstances() const;
	void getChildAtomInsts(set<AsscInstance*>& atoms) const;

	virtual void display(ostream&) const;
	virtual bool isCompositeInstance() const;
	virtual void reset();

protected:
	// assume key bound to key of template in CompFragment
	InstMap _componentInstances;
	
};

}

#endif /*COMPFRAGMENT_H_*/
