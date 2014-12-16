#ifndef DRIVER_H_
#define DRIVER_H_

#include "Logging.h"

namespace adn {

#define MKEY_ACTIONCMD "ac"

#define ACT_HELP "help"
#define ACT_CONSOLE "console"
#define ACT_QUIT "quit"
#define ACT_STATUS "status"

#define ACT_QUEUE "queue"
#define ACT_RUNQUEUE "runqueue"
#define ACT_CLEARQUEUE "clearqueue"

#define ACT_RUN "run"
#define ACT_RESET "reset"

#define ACT_ADDEL "addelement"
#define ACT_FREEZE "freeze"
#define ACT_SHOWPROC "showprocessing"

#define USAGE_QUIT "- exits the given console"

#define USAGE_QUEUE "command - adds given command to the action queue (eg queue setsize 3 3)"
#define USAGE_RUNQUEUE "[exit]"

#define USAGE_ADDEL "[elementstring]"
#define USAGE_FREEZE "[true|false]"
#define USAGE_SHOWPROC "[value]"

#define IKEY_EPOCHSPEREVAL "epochsPerEval"
#define IKEY_SHOWGAMEINTERVAL "showGameInterval"

#define IKEY_BATCHSIZE "batchSize"
#define IKEY_TESTCHANNEL "testOutputChannel"
#define IKEY_PREDCHANNEL "predictionChannel"
#define IKEY_MODELCHANNEL "modelChannel"
#define IKEY_EPOCHSPERWRITE "epochsPerWrite"

#define IKEY_PREDCLASSES "predictionClasses"
#define SKEY_HARNESSTYPE "harnessType"
#define SKEY_WORLDTYPE "worldType"

#define ACT_EVALUATE "evaluate"
#define ACT_READMODEL "readModel"
#define ACT_WRITEMODEL "writeModel"

using namespace logger;

void initialiseRNG();

class Driver;

typedef bool (*ActionFnPtr) (Driver&,vector<string>&);

struct ActionFn {
	ActionFn(int i = 0, ActionFnPtr f = NULL, string u = "") : params(i), fn(f), usage(u) {}
	unsigned int params;
	ActionFnPtr fn;
	string usage;
};

#define AVGRATE 0.01

class Driver : public Logging
{
public:
	Driver();
	virtual ~Driver();

	virtual void initialise(); // post constructor so subclass methods can run
	virtual bool verifyActionQueue();
	virtual bool verifyAction(string actionCommand);
	virtual void runActionQueue();
	virtual void doAction(string actionCommand);

    void setWorld(World* w);
    void setHarness(Harness* h);

    Harness* _harness;
    World* _world;

    bool _showConsole;

    const tr1::unordered_map<string, ActionFn>& getActionFunctions();

//protected:
    bool _initialised;

    list<string> _actionQueue;

    // game object reference pointers

    tr1::unordered_map<string, ActionFn> _actionFunctions;
    void defineActionFunctions();

    void startRun();
    void initRun();
    void finishRun();

    // parameters
    int _epochsPerEval;
    int _epochsPerWrite;
    int _outputIvl;

    int _testChannel;
    int _predChannel;
    int _writeChannel;

	int _predictionClasses; // show as continuous or just class

    void runEvaluation();
	void runPrediction();
    JobThread* getFreeThread();

	virtual void checkHarness();
	virtual void checkWorld();
	virtual bool mkHarness(const string& hname);
	virtual bool mkWorld(const string& wname);
	virtual void mkBatches();
	int runBatch(int& correctCounts, double& err, int set);
	vector<InstInfo*> _batch;
	int _batchSize; // for (distributed) mini-batch learning

	unsigned long int _totinsts;
	double _lastTrain;
	double _lastEval;


	map<const Associating*, map<const Associating*, int> > _confMatrix;

};

bool DoHelp(Driver& d, vector<string>& parameters);
bool DoConsole(Driver& d, vector<string>& parameters);
bool DoQuit(Driver& d, vector<string>& parameters);
bool DoStatus(Driver& d, vector<string>& parameters);
bool DoQueue(Driver& d, vector<string>& parameters);
bool DoRunQueue(Driver& d, vector<string>& parameters);
bool DoClearQueue(Driver& d, vector<string>& parameters);
bool DoRun(Driver& d, vector<string>& parameters);
bool DoRunThreaded(Driver& d, vector<string>& parameters);
bool DoReset(Driver& d, vector<string>& parameters);
bool DoAddElement(Driver& d, vector<string>& parameters);
bool DoShowProc(Driver& d, vector<string>& parameters);
bool DoSetFreeze(Driver& d, vector<string>& parameters);
bool DoRndPop(Driver& d, vector<string>& parameters);
bool DoUpdateParam(Driver& d, vector<string>& parameters);
bool DoClearAssocs(Driver& d, vector<string>& parameters);
bool DoResetSched(Driver& d, vector<string>& parameters);
bool DoClearPop(Driver& d, vector<string>& parameters);
bool DoTrimPop(Driver& d, vector<string>& parameters);
bool DoWriteModel(Driver& d, vector<string>& parameters);
bool DoReadModel(Driver& d, vector<string>& parameters);
//bool DoRunEpoch(Driver& d, vector<string>& parameters);
bool DoEvaluate(Driver& d, vector<string>& parameters);
bool DoTestSet(Driver& d, vector<string>& parameters);

bool getTrueFalse(string& phrase, bool& result);

}

#endif /*DRIVER_H_*/
