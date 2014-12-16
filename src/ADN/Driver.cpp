#include "ADN.h"
#include "CTokenizer.h"
#include <iomanip>      // std::setprecision
#include <fstream>

/*  started attempt to extract common elements from DBDriver into reusable
 * driver class.  handling players, tournaments and basic commands could
 * be done at a generic level.  defining functions to call is not straightforward
 * as defn of function pointer seems to require specification of class that
 * function belongs to, which does not lend well to polymorphism. */



//#include "ParamController.h"

namespace adn {


Driver::Driver()
{
    // read parameters
    _actionQueue = ParameterParser::_pinstance->getList(MKEY_ACTIONCMD);
    _showConsole = false;

    _initialised = false;

    _world = NULL;
    _harness = NULL;

    defineActionFunctions();

    _regInt(_batchSize, IKEY_BATCHSIZE);
    _regIntOp(_testChannel, IKEY_TESTCHANNEL, 0);
    _regIntOp(_predChannel, IKEY_PREDCHANNEL, 0);
    _regIntOp(_writeChannel, IKEY_MODELCHANNEL, 0);
    _regIntOp(_predictionClasses, IKEY_PREDCLASSES, 0);
    _regInt(_epochsPerWrite, IKEY_EPOCHSPERWRITE);
}

Driver::~Driver()
{
}

// ^\s*([^(\n]*)\(Driver\& parent\) *: *Action\(parent, *([0-9]+) *, *([^),]*) *, *([^)]*) *\) *\{ *\}
// \t_actionFunctions\[\3] = ActionFn(\2,\1,\4)
void Driver::defineActionFunctions() {
	_actionFunctions[ACT_HELP] 			= ActionFn(0,&DoHelp);
   	_actionFunctions[ACT_CONSOLE] 		= ActionFn(0,&DoConsole);
   	_actionFunctions[ACT_QUIT] 			= ActionFn(0,&DoQuit,USAGE_QUIT);
   	_actionFunctions[ACT_STATUS] 		= ActionFn(0,&DoStatus);
	_actionFunctions[ACT_QUEUE] 		= ActionFn(1,&DoQueue,USAGE_QUEUE);
	_actionFunctions[ACT_RUNQUEUE] 		= ActionFn(0,&DoRunQueue,USAGE_RUNQUEUE);
	_actionFunctions[ACT_CLEARQUEUE] 	= ActionFn(0,&DoClearQueue);
	_actionFunctions[ACT_RUN] 			= ActionFn(0,&DoRun);
	_actionFunctions[ACT_RESET] 		= ActionFn(0,&DoReset);
	_actionFunctions[ACT_ADDEL] 		= ActionFn(1,&DoAddElement,USAGE_ADDEL);
	_actionFunctions[ACT_FREEZE] 		= ActionFn(1,&DoSetFreeze,USAGE_FREEZE);
	_actionFunctions[ACT_SHOWPROC] 		= ActionFn(0,&DoShowProc,USAGE_SHOWPROC);
	_actionFunctions["createRndPop"] 	= ActionFn(0,&DoRndPop);
	_actionFunctions["updateParam"] 	= ActionFn(2,&DoUpdateParam,"param value");
	_actionFunctions["clearAssocs"] 	= ActionFn(0,&DoClearAssocs);
	_actionFunctions["resetsched"] 		= ActionFn(0,&DoResetSched);
	_actionFunctions["clearpop"] 		= ActionFn(0,&DoClearPop);
	_actionFunctions["trimpop"] 		= ActionFn(0,&DoTrimPop);
	//_actionFunctions["runEpoch"] 		= ActionFn(2,&DoTrimPop, "training? testEpoch? (each [true|false])");
	_actionFunctions[ACT_WRITEMODEL] 		= ActionFn(0,&DoWriteModel);
	_actionFunctions[ACT_READMODEL] 		= ActionFn(0,&DoReadModel);
	_actionFunctions[ACT_EVALUATE] 		= ActionFn(0,&DoEvaluate);
	_actionFunctions["validate"] 		= ActionFn(0,&DoTestSet);
}

void Driver::initialise() // post constructor so subclass constructor methods can run first
{
    _actionQueue = ParameterParser::getInstance().getList(MKEY_ACTIONCMD);

    _showConsole = false;
    _initialised = true;

	_regInt(_epochsPerEval, IKEY_EPOCHSPEREVAL);
	_regInt(_outputIvl, IKEY_SHOWGAMEINTERVAL);

	if (_seekDef(SKEY_WORLDTYPE)) {
		string type = _findStr(SKEY_WORLDTYPE);
		mkWorld(type);
	}
	if (_seekDef(SKEY_HARNESSTYPE)) {
		string type = _findStr(SKEY_HARNESSTYPE);
		mkHarness(type);
	}
}

bool Driver::verifyActionQueue()
{
	if (! _initialised) initialise();

    logout << "Verifying queued actions" << endl;
    writeBuf();

	// read actions and execute
    int failures = 0;
    list<string>::iterator iter;
    _foreach(_actionQueue, iter) {
        string thisActionString = *iter;

        if (! verifyAction(thisActionString)) failures++;
    }
    if (failures > 0) {
    	logout << "Errors in action queue." << endl;
    	writeBufError();
    	return false;
    }
    return true;
}

bool Driver::verifyAction(string actionCommand)
{
	if (! _initialised) initialise();
    logout << "action: " << actionCommand << endl;
    writeBuf();

    vector<string> tokens;

    CTokenizer<CIsSpace>::tokenize(tokens, actionCommand, CIsSpace());
    if (tokens.empty()) return true;

    // evaluate this action
    string& thisActionKey = tokens[0];

    bool result = true;
    tr1::unordered_map<string, ActionFn>::iterator fIter = _actionFunctions.find(thisActionKey);
    if (fIter != _actionFunctions.end()) {
        ActionFn& a = fIter->second;

    	if (tokens.size() - 1 < a.params) {
            logout << "Invalid use of command " << thisActionKey << endl;
            if (! a.usage.empty()) logout << "Usage: " << thisActionKey << " " << a.usage << endl;
            writeBufWarn();
            result = false;
    	}
    } else {
    	logout << "Unknown command: " << thisActionKey << endl;
    	writeBufWarn();
        result = false;
    }
    return result;
}

void Driver::runActionQueue()
{
	if (! _initialised) initialise();

	logout << "Running action queue" << endl;
	writeBuf();

    // read actions and execute
    while (! _actionQueue.empty()) {
        string thisActionString = _actionQueue.front();
        _actionQueue.pop_front();

        doAction(thisActionString);
    }
}

void Driver::doAction(string actionCommand)
{
	if (! _initialised) initialise();
    logout << "action: " << actionCommand << endl;
    writeBuf();

    vector<string> tokens;

    CTokenizer<CIsSpace>::tokenize(tokens, actionCommand, CIsSpace());
    if (tokens.empty()) return;

    // evaluate this action
    string& thisActionKey = tokens[0];

    bool result = true;
    tr1::unordered_map<string, ActionFn>::iterator fIter = _actionFunctions.find(thisActionKey);
    if (fIter != _actionFunctions.end()) {
        ActionFn& a = fIter->second;

    	if (tokens.size() - 1 >= a.params) {
    		// run command
    		result = (*a.fn)(*this, tokens);
            if (! result) {
            	logout << "Action command failed: " << thisActionKey << endl;
            	if (! a.usage.empty()) logout << "Usage: " << thisActionKey << " " << a.usage << endl;
            	writeBufWarn();
            }
    	} else {
            logout << "Invalid use of command " << thisActionKey << endl;
            if (! a.usage.empty()) logout << "Usage: " << thisActionKey << " " << a.usage << endl;
            writeBufWarn();
            result = false;
    	}
    } else {
    	logout << "Unknown command: " << thisActionKey << endl;
    	writeBufWarn();
        result = false;
    }
}

void Driver::setWorld(World* w) {
	// assume ownership of world
    if (_world) delete _world;
    _world = w;

    if (_harness) _harness->_worldPtr = w;
}

void Driver::setHarness(Harness* h) {
	if (_harness) delete _harness;
	_harness = h;

	if (_world && _harness) _harness->_worldPtr = _world;
}

void Driver::checkHarness() {
	if (! _harness) {
		bool found = false;
		if (_seekDef(SKEY_HARNESSTYPE)) {
			string type = _findStr(SKEY_HARNESSTYPE);
			found = mkHarness(type);
		}
		if (! found) {
			logout << "Harness not initialised" << endl;
			writeBufError();
		}
	}
}
void Driver::checkWorld() {
	if (! _world) {
		// check if world param defined
		bool found = false;
		if (_seekDef(SKEY_WORLDTYPE)) {
			string type = _findStr(SKEY_WORLDTYPE);
			found = mkWorld(type);
		}
		if (! found) {
			logout << "World not initialised" << endl;
			writeBufError();
		}
	}
}

void Driver::initRun() {
	checkWorld();
	checkHarness();
	_harness->initialise(); // sets up some properties from world

	// grab epoch size
	logout << "Starting run, epoch size " << _world->_epochSize
		<< " , total epochs " << Params::_pinstance->_maxEpochs << endl;
	writeBuf();

	_world->reset();
	_harness->resetState();
	_harness->_worldPtr = _world;

	mkBatches();
	JobThread::mkThreads();

	if (_harness->_removalIterations <= 1 * _batchSize) {
		logout << "Removal of features will take place after each batch, this can prevent development (each new feature only does one instance). Consider making removalIterations > 1x batch size" << endl;
		writeBufWarn();
	}
	//double runningAvg = 0.0;
	//bool first = true;
	_totinsts = 0;
}

void Driver::finishRun() {
	JobThread::clearThreads();

	vector<InstInfo*>::iterator iter;
	_foreach(_batch, iter) delete *iter;
	_batch.clear();
}

#define RANDOMALPHA 0.01
void Driver::startRun() {
	initRun();

	double thisRate = 0.0;
	bool cycle = false;
	int e = 0;
	if (Params::_pinstance->_trainTime) {
		// check if still time
		using namespace boost::posix_time;
		ptime now = second_clock::local_time();
		if (now > Params::_pinstance->_endTime) {
			logout << "out of time" << endl;
			writeBuf();
			cycle = false;
		} else {
			cycle = true;
			logout << "Timed run, elapsed " << now << ", " << Params::_pinstance->_trainTime << " minutes total" << endl;
			writeBuf();
		}
	}
	else if (e < Params::_pinstance->_maxEpochs) cycle = true;
	while (cycle) {
		logout << "Epoch " << e;
		if (! Params::_pinstance->_trainTime) logout << " / " << Params::_pinstance->_maxEpochs;
		logout << endl;
		writeBuf();

		// run epoch
		_world->reset();

		// unfreeze
		_harness->_freezeDevelopment = false;
		_world->start(trainSet);
		unsigned int s=0;
		unsigned int correctCount = 0;
		unsigned int nextOutput = 0;
		unsigned int nextRemoval = 0;
		unsigned int nextSchedule = 0;
		unsigned int prevSchedule = 0;
		double err, thiserr;
		while (s < _world->_epochSize) {
			int thisCorrect;
			int insts = runBatch(thisCorrect, thiserr, true);
			if (e==0 && s==0) err = thiserr;
			s += insts;
			_totinsts += insts;
			if (s >= nextRemoval) {
				_harness->maintainFragments();
				nextRemoval += _harness->_removalIterations;

				// clear up links to removed frags in batch stores (just memory handling,
				// was avoiding deleting and creating entries on every instance)
				for (int n=0; n < _batchSize; ++n) _batch[n]->clearMaps();
			}

			_harness->statCollector().doOutputStats(s);

			correctCount += thisCorrect;

			double upd = (_totinsts ? _max(AVGRATE, _batchSize / float(_totinsts)) : AVGRATE);
			thisRate += (thisCorrect / float(_batchSize) - thisRate) * upd;
			err += (thiserr - err) * upd;

			// update learning schedule
			if (Params::_pinstance->_scheduleRate && s >= nextSchedule) {
				Params::_pinstance->updateSchedule(_world->_epochSize, s - prevSchedule);
				nextSchedule += Params::_pinstance->_updateSteps;
				prevSchedule = s;
			}

			// display status of games
			if (s >= nextOutput) {
				logout << "insts " << s << " (" << _totinsts << ") ";
				_harness->display(logout);
				logout << " err " << err << " rate " << thisRate << endl;
				writeBuf();

				nextOutput += _outputIvl;
			}

			if (Params::_pinstance->_trainTime) {
				// check if still time
				using namespace boost::posix_time;
				ptime now = second_clock::local_time();
				if (now > Params::_pinstance->_endTime) {
					logout << "out of time" << endl;
					writeBuf();
					cycle = false;
					break; // exit training loop (try to do another test/eval before exit)
				}
			}
		}
		// include last instances in schedule
		Params::_pinstance->updateSchedule(_world->_epochSize, s - prevSchedule);

		double ratio = 0.0;
		if (s > 0) ratio = correctCount / (float) s;
		logout << "Training: " << correctCount << " / " << s << " " << ratio << endl;
		writeBuf();

		_lastTrain = ratio;

		// write model
		if (_epochsPerWrite && e % _epochsPerWrite == 0 && _writeChannel) {
	        logout << "== " << " epoch " << e << endl;
			_harness->writeModel(logout);
			writeBuf(_writeChannel, TRACE);
		}


		// run test mode
		if (_epochsPerEval && e % _epochsPerEval == 0) runEvaluation();

		e++;
		if (! Params::_pinstance->_trainTime) {
			cycle = (e < Params::_pinstance->_maxEpochs);
		}
	}

	// show final eval (repeat of last)
	logout << "Final: Training " << _lastTrain << " Test r: " << _lastEval << endl;
	writeBuf();

	finishRun();
}

int Driver::runBatch(int& correctCounts, double& err, int set) {

	correctCounts = 0;
	int batchSize = _min(_batchSize, _world->remainingInsts(set));
	for (int n=0; n < batchSize; ++n) {
		InstInfo* t = _batch[n];

		{
			// set up world instance (lock until queued)
			if (Params::_pinstance->_nThreads) {
				boost::mutex::scoped_lock tlock(t->_writeLock);
				_world->createInstance(*t, set);
			} else {
				_world->createInstance(*t, set);
			}
		}

		Task d;
		d._job = jobProcInstance;
		d._training = (set == trainSet);
		d._curInst = t;

		if (Params::_pinstance->_nThreads) {
			boost::mutex::scoped_lock lock(JobThread::queue_mut);
			JobThread::job_queue.push_back(d);
			JobThread::queue_change.notify_one();
		} else {
			// running without threads
			t->processInstance(d);
		}
	}

	// wait for any busy threads to finish
	if (Params::_pinstance->_nThreads) {
		JobThread::waitQueueFinished();
		JobThread::waitAllIdle();
	}

	// collect results
	err = 0.0;
	for (int n=0; n < batchSize; ++n) {
		if (_batch[n]->_correct) correctCounts++;
		if (set == testSet) _confMatrix[_batch[n]->_lastGuess][_batch[n]->_currentClass]++;
		err += (_batch[n]->_err - err) / double(n+1);
	}

	// apply updates
	if (set == trainSet) {
		_harness->applyChanges(_batch, batchSize);
	}

	// output predictions (should be in order)
	if ((set == testSet && _testChannel) || (set == predSet && _predChannel)) {
		writeBuf();
		vector<const Associating*>::iterator citer;
		for (int n=0; n < batchSize; ++n) {
			InstInfo* t = _batch[n];
			if (_world->getBinaryClass()) {
				// just output the one class value
				if (_predictionClasses) {
					pair<Associating*, Associating*> cls = _world->getBinaryClasses();
					if (t->_outputs[cls.second] < Params::_NEUTRVAL) cls.first->display(logout);
					else cls.second->display(logout);
				} else {
					logout << t->_outputs[_world->getBinaryClasses().second];
				}
			} else {
				if (_predictionClasses) {
					// find highest value class
					double maxVal = 0.0;
					const Associating* maxClass = NULL;
					for (int c=0; c<_world->_allClasses.size(); ++c) {
						const Associating* cls = _world->_allClasses[c];
						if (c == 0 || t->_outputs[cls] > maxVal) {
							maxVal = t->_outputs[cls];
							maxClass = cls;
						}
					}
					if (maxClass) maxClass->display(logout);
				} else {
					// output each
					for (int c=0; c<_world->_allClasses.size(); ++c) {
						if (c) logout << ",";
						logout << t->_outputs[_world->_allClasses[c]];
					}
				}
			}
			logout << endl;
			if (set == testSet)	writeBuf(_testChannel);
			if (set == predSet)	writeBuf(_predChannel);
		}
	}
	return batchSize;
}

bool Driver::mkHarness(const string& hname)
{

}

bool Driver::mkWorld(const string& wname)
{

}

void Driver::mkBatches() {
	// subclass should create batch objects

}

void Driver::runEvaluation() {
	// run epoch
	_world->reset();
	_confMatrix.clear();

	_harness->_freezeDevelopment = true;
	_world->start(testSet);
	unsigned int s=0;
	int correctCount = 0;
	int nextOutput = 0;
	logout << "Running on test set" << endl;
	writeBuf();
	if (_testChannel) {
		logout << "== iter " << _totinsts << endl;
		writeBuf(_testChannel);
	}
	double err;
	while (s < _world->_testSize) {
		int thisCorrect;
		s += runBatch(thisCorrect, err, testSet);
		_harness->statCollector().doOutputStats(s);
		correctCount += thisCorrect;

		// display status of games
		if (s >= nextOutput) {
			logout << "insts " << s << " ";
			_harness->display(logout);
			logout << " err " << err << " rate " << correctCount / double(s) << endl;
			writeBuf();

			nextOutput += _outputIvl;
		}

	}

	double ratio = 0.0;
	if (s > 0) ratio = correctCount / (float) s;
	logout << "Evaluation: " << correctCount << " / " << s << " " << ratio << endl;
	writeBuf();

	// find predictions
	if (_world->_predSize) {
		logout << "Running on prediction set" << endl;
		writeBuf();
		_world->start(predSet);
		s = 0;
		nextOutput = 0;
		if (_predChannel) {
			logout << "== iter " << _totinsts << endl;
			writeBuf(_predChannel);
		}
		while (s < _world->_predSize) {
			int thisCorrect;
			s += runBatch(thisCorrect, err, predSet);
			_harness->statCollector().doOutputStats(s);

			// display status of games
			if (s >= nextOutput) {
				logout << "insts " << s << " ";
				_harness->display(logout);
				logout << endl;
				writeBuf();

				nextOutput += _outputIvl;
			}
		}
	}

	if (_world->_testSize) {
		// show confusion matrix
		logout << "Confusion matrix: (rows=predictions, cols=actual)" << endl;
		//logout << setprecision(2);
		vector<const Associating*>::iterator riter, citer, niter;
		tr1::unordered_map<const Associating*, int> tp_counts, tn_counts, c_counts, n_counts;
		_foreach(_world->_allClasses, riter) {
			const Associating* predict = *riter;
			predict->display(logout);
			logout << ":";
			_foreach(_world->_allClasses, citer) {
				const Associating* actual = *citer;
				int counts = _confMatrix[predict][actual];
				logout << " " << counts / float(_world->_testSize);
				c_counts[actual] += counts;
				if (predict == actual) tp_counts[actual] += counts;
				_foreach(_world->_allClasses, niter) {
					n_counts[*niter] += counts;
					if (predict != *niter) tn_counts[*niter] += counts;
				}
			}
			logout << endl;
		}
		//logout.unsetf(ios_base::floatfield); // clear precision

		logout << "Class accuracy:";
		_foreach(_world->_allClasses, riter) {
			logout << " ";
			(*riter)->display(logout);
		}
		logout << endl << "counts: ";
		_foreach(_world->_allClasses, riter) {
			logout << " " << tp_counts[*riter] << "/" << c_counts[*riter];
		}
		logout << endl << "accuracy: ";
		double cprod = 1.0;
		_foreach(_world->_allClasses, riter) {
			logout << " " << tp_counts[*riter] / float(c_counts[*riter]);
			cprod *= tp_counts[*riter] / float(c_counts[*riter]);
		}
		logout << endl << "Class result product:" << cprod << endl;
		writeBuf();

		logout << "TPR/TNR for classes ";
		_foreach(_world->_allClasses, riter) {
			(*riter)->display(logout);
			logout << " ";
		}
		logout << endl << "TPR: ";
		int t_tp = 0, t_tn = 0, t_t = 0, t_n = 0;
		_foreach(_world->_allClasses, riter) {
			logout << tp_counts[*riter] / float(c_counts[*riter]) << " ";
			t_tp += tp_counts[*riter];
			t_t += c_counts[*riter];
		}
		float tpr = t_tp / float(t_t);
		logout << "(" << tpr << ")" << endl;
		logout << "TNR: ";
		_foreach(_world->_allClasses, riter) {
			logout << tn_counts[*riter] / float(n_counts[*riter]) << " ";
			t_tn += tn_counts[*riter];
			t_n += c_counts[*riter];
		}
		float tnr = t_tn / float(t_n);
		logout << "(" << tnr << ")" << endl;
		logout << "TPRxTNR: " << tpr * tnr;
		writeBuf();
	}

	_lastEval = ratio;
	_harness->_freezeDevelopment = false; // prob not needed
}

void Driver::runPrediction()
{
	// called to just run a single prediction run of the prediction set,
	// and write predictions to specified file
	_world->reset();
	_confMatrix.clear();

	_harness->_freezeDevelopment = true;

	// find predictions
	if (_world->_predSize == 0) {
		logout << "no prediction set" << endl;
		writeBufError();
	}

	// assume output to _predChannel is defined

	logout << "Running on prediction set" << endl;
	writeBuf();
	_world->start(predSet);
	int s = 0;
	int nextOutput = 0;
	double err;
	while (s < _world->_predSize) {
		int thisCorrect;
		s += runBatch(thisCorrect, err, predSet);

		// display status of games
		if (s >= nextOutput) {
			logout << "insts " << s << " ";
			_harness->display(logout);
			writeBuf();

			nextOutput += _outputIvl;
		}

	}
	_harness->_freezeDevelopment = false; // prob not needed
}

const tr1::unordered_map<string, ActionFn>& Driver::getActionFunctions()
{
	return _actionFunctions;
}

bool DoTrimPop(Driver& d, vector<string>& parameters)
{
	d._harness->maintainFragments();
	return true;
}


bool DoHelp(Driver& d, vector<string>& parameters)
{
    // show each command and usage
    logout << endl << "Available commands:" << endl << "--------------" << endl;
    tr1::unordered_map<string, ActionFn>::iterator aIter;
    _foreach(d._actionFunctions, aIter) {
    	ActionFn& a = aIter->second;
    	logout << aIter->first << " " << a.usage << endl;
    }
    writeBuf();
    return true;
}

bool DoStatus(Driver& d, vector<string>& parameters)
{
    logout << "World:" << endl;
    if (d._world) logout << d._world->getStateString() << endl;
    else logout << "(null)" << endl;

    logout << "Action Queue:" << endl;
    const list<string>& queue = d._actionQueue;

    list<string>::const_iterator q_iter;
    _foreach(queue, q_iter) logout << *q_iter << endl;

    writeBuf();
    return true;
}

bool DoClearQueue(Driver& d, vector<string>& parameters)
{
	d._actionQueue.clear();
    return true;
}

bool DoQuit(Driver& d, vector<string>& parameters)
{
	d._showConsole = false;
    return true;
}


bool DoRun(Driver& d, vector<string>& parameters)
{
    d.startRun();
    return true;
}

bool DoReset(Driver& d, vector<string>& parameters)
{
    if (d._harness) d._harness->resetState();
    return true;
}

bool DoQueue(Driver& d, vector<string>& parameters)
{
    // record parameters as action to queue
    ostringstream strcat;
    for (unsigned int i=1; i<parameters.size(); i++) {
        if (i != 1) strcat << " ";
        strcat << parameters[i];
    }

    d._actionQueue.push_back(strcat.str());
    return true;
}

bool DoRunQueue(Driver& d, vector<string>& parameters)
{
    if (parameters.size() > 1 && parameters[1] == "exit") d._showConsole = false;

    d.runActionQueue();
    return true;
}

bool DoConsole(Driver& d, vector<string>& parameters)
{
	d._showConsole = true;

    while(d._showConsole) {
        logout << "command (or help) > ";
        writeBuf();

        string response;
        getline(cin,response);
        d.doAction(response);
    }
    return true;
}

bool DoSetFreeze(Driver& d, vector<string>& parameters)
{
	d.checkHarness();
    // parameters: name type
    unsigned int freezeParam = 1;

    bool newFreezeVal = true;
    string& freezeCmd = parameters[freezeParam];

	if (! getTrueFalse(freezeCmd, newFreezeVal)) {
		logout << "invalid freeze command: " << freezeCmd << ", should be true or false" << endl;
		writeBuf();
		return false;
	}
    if (d._harness) d._harness->_freezeDevelopment = newFreezeVal;
    else {
    	warn("Harness not defined\n");
    	return false;
    }

    return true;
}

bool DoShowProc(Driver& d, vector<string>& parameters)
{
	d.checkHarness();
    // parameters: name type
    unsigned int showProcParam = 1;

	string& showProcCmd = parameters[showProcParam];
	bool showProcVal;
	if (! getTrueFalse(showProcCmd, showProcVal)) {
		logout << "invalid command: " << showProcCmd << ", should be true or false" << endl;
		writeBuf();
		return false;
	}

    if (d._harness) d._harness->_showProcessing = showProcVal;
    else {
    	warn("Harness not defined\n");
    	return false;
    }

    return true;
}


bool DoAddElement(Driver& d, vector<string>& parameters)
{
	d.checkHarness();
    // concatenate remaining entries (with " ")
    ostringstream concat;
    unsigned int i;
    for(i=2; i<parameters.size(); ++i) {
    	if (i!=1) concat << " ";
    	concat << parameters[i];
    }
    if (d._harness) d._harness->addElement(concat.str());
    else {
    	warn("Harness not defined\n");
    	return false;
    }
    return true;
}

bool DoRndPop(Driver& d, vector<string>& parameters)
{
	d.checkHarness();

    if (d._harness) d._harness->createRndPop();
        else {
        	warn("Harness not defined\n");
        	return false;
        }
    return true;
}

bool DoUpdateParam(Driver& d, vector<string>& parameters)
{
    // parameters: name type
    unsigned int paramName = 1;
    unsigned int paramVal = 2;

    return ParameterParser::getInstance().updateParam(parameters[paramName], parameters[paramVal]);
}


bool getTrueFalse(string& phrase, bool& answer) {
	if (phrase == "true" || phrase == "t" || phrase == "1") {
		answer = true;
		return true;
	}
	if (phrase == "false" || phrase == "f" || phrase == "0") {
		answer = false;
		return true;
	}
	else return false;
}

bool DoClearAssocs(Driver& d, vector<string>& parameters)
{
	d.checkHarness();

    if (! d._harness) {
    	logout << "harness not present" << endl;
  	    writeBufWarn();
	    return false;
    }

    d._harness->clearAllAssocs();
    return true;
}
bool DoResetSched(Driver& d, vector<string>& parameters)
{
    Params::_pinstance->resetSchedule();

    return true;
}

bool DoClearPop(Driver& d, vector<string>& parameters) {
	d.checkHarness();
    if (! d._harness) {
    	logout << "Harness not found, invalid use";
    	writeBufWarn();
    	return false;
    }

    d._harness->clearPopulation();
    return true;
}
/*bool DoRunEpoch(Driver& d, vector<string>& parameters) {
	unsigned int trainingParam = 1;
	unsigned int testParam = 2;
	bool train, test;
	if (parameters.size() < 3) return false;
	if (! getTrueFalse(parameters[trainingParam], train)) return false;
	if (! getTrueFalse(parameters[testParam], test)) return false;

	if (train) d.runTraining();
	if (test) d.runEvaluation();

    return true;
}*/

bool DoWriteModel(Driver& d, vector<string>& parameters) {
	d.checkHarness();
	int fileParam = 1;

	std::ofstream fs;
	fs.open(parameters[fileParam].c_str());

	if (fs.good()) {
		d._harness->writeModel(fs);
	} else {
		logout << "Error writing model to file " << parameters[fileParam] << endl;
		writeBufWarn();
		return false;
	}
	fs.close();
	return true;
}
bool DoReadModel(Driver& d, vector<string>& parameters) {
	d.checkHarness();
	int fileParam = 1;

	std::ifstream ifs;
	ifs.open(parameters[fileParam].c_str());

	if (ifs.good()) {
		d._harness->readModel(ifs);
	} else {
		logout << "Error reading model from file " << parameters[fileParam] << endl;
		writeBufWarn();
		return false;
	}
	ifs.close();
	return true;
}

bool DoEvaluate(Driver& d, vector<string>& parameters)
{
	d.initRun();
    d.runPrediction();
	d.finishRun();
    return true;
}
bool DoTestSet(Driver& d, vector<string>& parameters)
{
	// called to just run a single prediction run of the prediction set,
	// and write predictions to specified file
	d.initRun();
	d.runEvaluation();
	d.finishRun();
    return true;
}


} // namespace dotsnboxes
