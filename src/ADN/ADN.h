#ifndef ADN_H_
#define ADN_H_

/*
 * Module to implement Abstract Deep Network code
 */

#include "macros.h"
#include "ParameterParser.h"
#include "ExternalIO.h"
#include "Params.h"
#include "Associating.h"
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <vector>
#include <boost/thread.hpp>

namespace adn {
using namespace utils;
using namespace std;

// class definitions
class Fragment;
class CompFragment;
class Harness;
class World;
class AsscInstance;
class CompInstance;

typedef std::tr1::unordered_map<const Associating*, double> ClassMap;
typedef unsigned int uint;

struct FragDetails {
	int _match;
	double _activation;
	tr1::unordered_map<NeurParam*, vector<double> > _paramChanges;
	vector<const Associating*> _newAssocClasses;
	vector<double> _newAssocVals;
};

enum jobs {
	jobIdle = 0,
	jobProcInstance = 1,
	jobApplyUpdates = 2,
	jobTerminate = 3
};

struct Task;

struct InstInfo {

	tr1::unordered_set<Fragment*> _activeFragments;
	tr1::unordered_set<Fragment*> _activeAtomFragments;
	tr1::unordered_set<CompFragment*> _activeComposites;

	tr1::unordered_map<const Fragment*, FragDetails> _fragDetails;
	ClassMap _classActivations;
	ClassMap _outputs;
	ClassMap _classDeltas;
	ClassMap _classBiasMods;
	vector<Fragment*> _activationFrags;

	vector<Fragment*> _newAtoms;
	vector<CompFragment*> _newComps;

	const Associating* _currentClass;
	bool _correct;
	const Associating* _lastGuess;

	float _averageResult;
	int _avResUpdates;
	double _err;

    Harness* _harness;

	void reset();
	void clearMaps();

	boost::mutex _writeLock;

	void processInstance(Task& d);
	void processFrags(Task& d);
    void fragApplyChanges(Task& d, Fragment* frag);
};

struct Task {
	Task();
    int _job;

    // calling info for activation
    bool _training;
    InstInfo* _curInst;

	// calling info for process frags
    vector<InstInfo*>* _batch;
    vector<Fragment*>* _atoms;
    vector<CompFragment*>* _comps;
    int _low;
    int _high;
    int _batchSize;
};
struct JobThread {
	JobThread();

    bool _busy;

    Task _workingTask;

    void mainLoop();

	boost::mutex _stateLock;
	boost::condition_variable _stateChange;

	static vector<JobThread*> _jobThreads;
	static vector<boost::thread*> _threadHandles;
	//static JobThread* getFreeThread();
	static void waitQueueFinished();
	static void waitAllIdle();
	static void mkThreads();
	static void clearThreads();

	static boost::condition_variable queue_change;
	static boost::mutex queue_mut;
	static list<Task> job_queue;
};


}

#include "World.h"

#include "Fragment.h"
#include "CompFragment.h"
#include "Harness.h"
//#include "InstHarness.h"

#include "Driver.h"

#endif //FRAGMENTS_H_
