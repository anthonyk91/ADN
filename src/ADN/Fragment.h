#ifndef ASSCFRAGMENT_H_
#define ASSCFRAGMENT_H_

#include "Associating.h"
#include "Displaying.h"
#include "Observer.h"

#include <set>
#include <vector>

#define DIRECTCOUNTS 50

namespace adn {
using namespace std;

class AsscInstance;

using namespace utils;
class Fragment : public Displaying,
	public Associating
{
	friend class Harness;
public:    
	Fragment(Harness* parent, float biasVal = 0.0);
	Fragment(const Fragment& f); // copy const
	virtual ~Fragment();

    static bool assocNormGThan(const Fragment* a, const Fragment* b);

	virtual Fragment& operator=(const Fragment& f); // op=
	virtual bool operator==(const Fragment& f) = 0; // check if matches
	
	virtual bool findActivation(InstInfo& t) const;

	unsigned int activeInstances() const; // number of instances active
	const vector<AsscInstance*>& getInstances() const;
	virtual void clearInstances(); // removes references to instances
	void releaseInstances(); // attempts to delete instances
	unsigned int pruneInstances(unsigned int maxInstances); // returns number removed
	
	// for finding all children of a (composite) frag
	virtual void getChildSet(set<Fragment*>& recordSet);
	virtual void getChildSet(set<const Fragment*>& recordSet) const;
	virtual void getChildList(list<Fragment*>& recordList);
	virtual void getChildList(list<const Fragment*>& recordList) const;

	virtual void display(ostream&) const;

	const vector<CompFragment*>& getParents() const;
	void addParent(CompFragment* compFrag);
	void releaseParent(CompFragment* compFrag, bool allLinks = false);
	void clearParents();
	virtual bool isComposite() const;
	void removeParents();
	
	Harness* getParentHarness() const;
	void addInstance(AsscInstance* inst);
    int _fragID;

    //void setBiasVal(double bias);
    double getBiasVal() const;
    NeurParam& biasParam();
    //void setDroppedOut(bool d);
    //bool droppedOut() const;
    virtual void reset();

    void trackState(bool active, double learnRate);
    double findAssocNorm();
    double calcAssocNorm();
    double getAssocNorm() const;
    void updateAssocNorm(double val); //< set to given val if higher
    void addAssocNorm(double val); //< set to given val if higher
    void setAssocNorm(double val);

	virtual void doBackprop(InstInfo& t, double delta);

    double _matchFreq; // % of states encountered that feature matches
    int _statesObs;  // states encountered
    int _statesMatch; // states where frag matches

    int _linkDepth; // for identifying how deep structure is

    virtual void write(ostream&) const;
    virtual void read(istream&);
protected:
    NeurParam _biasParam; // used in sigmoid activation

    vector<CompFragment*> _parents; // record parent composites explicitly

    Harness* _parHarness;

    double _assocNorm;
	double _avNorm;

    unsigned int _numInstances;
    unsigned int _instTests;
    vector<AsscInstance*> _instantiations;
};

class AsscInstance : public utils::Displaying, public Subject<AsscInstance, CompInstance>
{
public:
	AsscInstance(Fragment& templateFrag) : _template(&templateFrag) {}
	virtual ~AsscInstance();
	Fragment& getTemplate() const;
	void setTemplate(Fragment&);
	
	int getObservingComposites() const;
	virtual void display(ostream&) const;
	virtual bool isCompositeInstance() const;
	virtual void reset();

protected:
	Fragment* _template;
};



}

#endif /*ASSCFRAGMENT_H_*/
