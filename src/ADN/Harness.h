#ifndef HARNESS_H
#define HARNESS_H

//Generic harness abstract class
//
//Anthony Knittel, Centre for the Mind, Australia
//anthony@centreforthemind.com
//July 2005

#include <vector>
#include "StatHandler.h"

namespace adn {
using namespace utils;

#define IKEY_RMVITER "removalIterations"
#define IKEY_MERGERULEINTERVAL "mergeRuleInterval"
#define IKEY_AVRWDALPHA "avResAlpha"
#define IKEY_SHOWPROC "showProcessing"

#define FKEY_CREATEINSTPROB "createEachInstProb"
#define FKEY_ATOMRATIO "atomRatio"
#define IKEY_MAXCOMPOSITES "maxComposites"
#define IKEY_MAXATOMFRAGMENTS "maxAtomFragments"
#define IKEY_TRACKACTIVITY "trackActivity"

#define IKEY_MAXCHILD "maxChildren"
#define IKEY_INSERTCHECK "insertEqCheck"
#define IKEY_PROPMETHOD "propagationMethod"
#define IKEY_RUNPOPADJUST "runPopAdjust"

#define FLOAT_LEARNRATE "learningRate"
#define INT_SCHEDRATE "scheduleRate"
#define FLOAT_MOMRATE "momentum"
#define INT_WTDECAY "weightDecay"
#define INT_OUTACTMETHOD "outActivationMethod"
#define INT_BACKPROPMETHOD "backpropMethod"
#define INT_ACTIVSET "activationSet"
#define INT_FRAGPOPMETHOD "fragPopMethod"
#define INT_CREATEFRAGS "createFrags"
#define FKEY_POSTCREATE "probPostCreate"
#define IKEY_MINDEPTH "outputMinDepth"
#define IKEY_SPARSELIM "sparseOutLimit"
#define FKEY_SPARSECREATEPROB "sparseCreateProb"
#define IKEY_NORMERROR "normaliseError"

// output channels (files to write to)
#define DUMPCOMPCHANNEL 13
#define DUMPATOMCHANNEL 14

#define CREATEBASICPROB 0.5
#define MINAVGREWARDSTEPS 50
#define MINWEIGHTS 0.0001
// this is just a check to prevent
// overflow on some exp(x) operations (assuming 64 bit doubles)
#define MAXEXP 600

// propagation (which frags to examine)
enum {
	propActive = 1,
	propAll = 2
};

// checking for equal frags on creation
enum {
	insertCheckAtoms = 1,
	insertCheckComps = 2,
};

// output activation method
enum assocSel {
	selSum = 2,
	selAvg = 3,
	selSoftMax = 4
};

// set of frags to use to activate outputs
enum activSet {
	actAllComps = 1,
	actActive = 2,
	actAllCompsAtoms = 4,
	actActiveCompsAtoms = 5
};

// backprop through the network
enum backpropMethods {
	bpComps = 1,
	bpPosition = 2,
	bpAtoms = 4
};

// population selection methods
enum fragPopMethods {
	assocNorm = 4,
	avNorm = 5,
	assocWtSum = 6
};


struct HarnessStats {
	HarnessStats() : _atomsCreated(0), _compsCreated(0), _lastActiveAtoms(0),
		_lastActiveComps(0), _maxExaminedIterations(0) {}

	int _atomsCreated;
	int _compsCreated;

	int _lastActiveAtoms;
	int _lastActiveComps;

	unsigned int _maxExaminedIterations;
	float _maxIterWeights;
	unsigned int _maxInsts;
	unsigned int _maxInstsExisting;
	unsigned int _maxInstsDefault;
};

class FragStatCollector;


typedef std::tr1::unordered_set<Fragment*> FragSet;

class Harness : public Displaying
{
public:
	friend class FragStatCollector;

	Harness();
	virtual ~Harness();

	void initialise();

	virtual const Associating* getClassification(InstInfo& t) const; // null if not successful
	virtual void postClassification(InstInfo& t) const; // learning
	virtual void resetState();
	virtual void applyChanges(vector<InstInfo*>& batch, int batchSize);

	virtual void clearPopulation(); //< removes all fragments and instances
	virtual void createRndPop();

	// fragment handling
    virtual void maintainFragments();

	virtual bool addFragment(Fragment* newFragment);
	virtual bool addComposite(CompFragment* newCompFragment);

	// output associations
	void clearAllAssocs(); //< reset assoc values of all frags
	void outputWeights(InstInfo& t, ostream& os) const;
	unsigned int getFragID(); // locks
	void incrAssocs(); // locks

    int _maxChildren;

	int _testMethod; // control propagation calls

    float _createEachInstProb;
    float _atomRatio;
    bool _keepChildOrder;
    int _insertEqCheck;
    int _trackActivity;
	int _removalIterations;

    World* _worldPtr;

	int _showProcessing;
	float _averageResult;
	bool _freezeDevelopment; //< prevent changes to agents that affect behaviour
    int _sparseAssocLimit;

	FragStatCollector& statCollector();
	const FragStatCollector& statCollector() const;

	virtual void display(ostream&) const;
	virtual void displayState(ostream& output) const;
	virtual bool addElement(const string& elstring); // parse string and add given element

	virtual void writeModel(ostream&) const;
	virtual void readModel(istream&);

    tr1::unordered_map<int, Fragment*> _idMap;

protected:
	virtual void observeState(InstInfo& t) const;
	double adjustFragOutput(InstInfo& t, Fragment* thisFrag) const;

	static float probOkCompFragments(int numFragments);
	virtual void createAtomFragments(InstInfo& t, vector<Fragment*>* newFrags = NULL) const;
	virtual void createCompFragments(InstInfo& t, vector<Fragment*>* newFrags = NULL) const;

    // fragment methods
    virtual void activateFrags(InstInfo& t) const;

	bool removeFragment(Fragment* newFragment);
	bool removeComposite(CompFragment* newCompFragment);
	void trimFragments();
	void trimComposites();

	virtual CompFragment* makeComposite(const vector<Fragment*>& chosenFragments);
	virtual CompFragment* makeComposite(const tr1::unordered_map<int, Fragment*>& chosenFragMap);

	virtual Fragment* readAtom(istream&);
	virtual CompFragment* readComp(istream&);

	boost::mutex _mutex; // write lock for threaded changes (fragID, numAssocs)

	vector<Fragment*> _atomFragments;
	vector<CompFragment*> _compFragments;

	int _numAtoms;
	int _numComposites;
	unsigned int _maxFragID;
    int _numAssocs;

	// output associations
	virtual const Associating* findOutputActivations(InstInfo& t) const; // handle parallel calls (merge w activateOutput?)
	void procOutputActivations(InstInfo& t, vector<Fragment*>::iterator iter, vector<Fragment*>::iterator last) const;
    void runBackprop(InstInfo& t) const;
    void updateAssoc(Associating* fragment, const Associating* classObj);
    virtual bool postCreateNew(InstInfo& t) const; //< create method after result given
    virtual void backpropagatePosition(list<CompFragment*>& fragOrder) const;
    void findAssocNorms();
    void findAssocWtSums();

    tr1::unordered_map<const Associating*, NeurParam> _classBiases;

	// hyperparameters
	int _sortIterations;
	float _avResAlpha;
	int _avResUpdates;

	int _maxComposites;
	int _maxAtomFragments;
    int _runPopAdjust;

    int _outActivationMethod;
    int _backpropMethod;
    int _activationSet;
    int _dumpFeatures;
    int _fragPopMethod;
    double _probPostAgentParam;
    int _minDepth;
    double _sparseCreateProb;
    int _errUpdates;
    bool _calcAvNorm;
    int _normError;

	// stats
	int _gameMove; //< number of moves into game
	int _gamesPlayed; //< number of resets that have taken place.  for handling longer-term parameters

	// some stats
	mutable HarnessStats _stats;
	FragStatCollector* _statCollector;

	const Associating* _chosenClass;

};

#define IKEY_DUMPFRAGSIVL "dumpFragsInterval"

class FragStatCollector : public StatCollector
{
public:
	FragStatCollector(const Harness& watchHarness);
	virtual ~FragStatCollector();

    virtual void doOutputStats(int iteration);

protected:
	string fragInfo();
    const Harness& _watchHarness;

	int _fragInfoChannel;
    int _dumpFragsInterval;

    int _nextDumpFrags;
};
#define IKEY_FRAGINFOCHANNEL "fragInfoChannel"

/*inline void doRecordChange(FragDetails& f, double& grad, NeurParam* param) {
	//f._paramChangeCounts[param]++;
	f._paramChanges[param] += grad;
}*/


} // namespace evagents


#endif // HARNESS_H
