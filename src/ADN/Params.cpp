/*
 * Params.cpp
 *
 *  Created on: 26/05/2014
 *      Author: anthony
 */

#include "ADN.h"
#include "Singleton.h"

namespace adn {
using namespace utils;

Params::Params()
{
	_Assert(_pinstance == NULL, ("Attempt to create second instance of singleton geneticparameters\n"));

	_pinstance = this;
}

Params::~Params()
{
	//dtor
	_pinstance = NULL;
}


Params* Params::_pinstance = NULL; // singleton pointer

int Params::_neurMethod = 0;
double Params::_MAXTARGET = 0.0;
double Params::_MINTARGET = 0.0;
double Params::_NEUTRVAL = 0.0;


Params& Params::instance()
{
	if (_pinstance == NULL) {
		// (discontinued)- construct basic object at this level, if need to
		// use more specific object later can inherit (via copy
		// const) from this one
		//_gpinstance = new Params();
		//error("Attempt to access non-instantiated base-created singleton\n");
        throw SingletonNoInstException("Params");
	}
	return *_pinstance;
}

void Params::setBaseValues()
{
    _regFlt(_reproductionBeta,FKEY_REPRBETA);

    _regInt(_neurMethod,INT_NEURMETHOD);
	// define targets (note if method is changed at these won't update)
    setupNeurMethod();

	_regDbl(_learnRate,FLOAT_LEARNRATE);
	_regDbl(_momRate,FLOAT_MOMRATE);
	_regDbl(_wtDecay,INT_WTDECAY);
	_regInt(_scheduleRate,INT_SCHEDRATE);
	_regInt(_maxEpochs, IKEY_MAXEPOCHS);
	_regDblCond(_learnInit, "learnInit", 0.0, _scheduleRate);
	_regDblCond(_learnDecay, "learningDecay", 1.0, _scheduleRate);
	_regDblCond(_momInit, "momInit", 1.0, _scheduleRate);
	_regDblCond(_momFinal, "momFinal", 1.0, _scheduleRate);
	_regIntCond(_updateSteps, "updateSteps", 0, _scheduleRate);
	_regIntCond(_fastEpochs, "fastEpochs", 0.0, _scheduleRate == perEpochMomentum);
	_regIntCond(_scheduleFixedSteps, "scheduleFixedSteps", 0, _scheduleRate == perStep || _scheduleRate == perTime);
	_regIntOp(_trainTime, IKEY_TRAINTIME, 0);
	if (_trainTime) {
		// set start and finish times
		using namespace boost::posix_time;
		_startTime = second_clock::local_time();
		_endTime = _startTime + minutes(_trainTime);
	}

	if (_scheduleRate) {
		_momRate = _momInit;
		_learnRate = _learnInit * _momRate;
	}

    _regInt(_nThreads, IKEY_NUMTHREADS);
    _stepsMade = 0;
}

void Params::setupNeurMethod() {
	switch (_neurMethod) {
	case neurtanh:
		_MAXTARGET = 0.95;
		_MINTARGET = -0.95;
		_NEUTRVAL = 0.0;
		break;
	case neurrelu:
		_MAXTARGET = 0.95;
		_MINTARGET = 0.0;
		_NEUTRVAL = 0.5;
		break;
	case neurlog:
		_MAXTARGET = 0.95;
		_MINTARGET = 0.05;
		_NEUTRVAL = 0.5;
		break;
	case neurlin:
		_MAXTARGET = 0.95;
		_MINTARGET = -0.95;
		_NEUTRVAL = 0.0;
		break;
	case neurretan:
		_MAXTARGET = 0.95;
		_MINTARGET = 0.0;
		_NEUTRVAL = 0.0;
		break;
	}
}

void Params::resetSchedule() {
	_stepsMade = 0;

	if (_trainTime) {
		// set start and finish times
		using namespace boost::posix_time;
		_startTime = second_clock::local_time();
		_endTime = _startTime + minutes(_trainTime);
	}
}

void Params::updateSchedule(int epochSize, int steps) {
	_stepsMade += steps;
	// update every _updateSteps
	float epochNum = _stepsMade / double(epochSize);

	if (_scheduleRate == perStep) {
		float stepBlocks = _stepsMade / float(_scheduleFixedSteps);
		_learnRate = _learnInit * pow(_learnDecay, stepBlocks);
	} else if (_scheduleRate == perEpoch) {
		_learnRate = _learnInit * pow(_learnDecay, epochNum);
	} else if (_scheduleRate == perEpochMomentum) {
		// method similar to that used in Hinton's dropout paper
		// end of "epoch" (hack, sampling items randomly)

		// custom adjustment.  (assume start with m=0.5 initially)
		if (epochNum <= _fastEpochs) {
			float ratio = epochNum / float(_fastEpochs);
			_momRate = _momInit * (1 - ratio) + _momFinal * ratio;
		}

		_learnRate = _learnInit * pow(_learnDecay, epochNum) * _momRate;
	} else if (_scheduleRate == perTime) {
		using namespace boost::posix_time;
		ptime now = second_clock::local_time();
		float percDone = ((now - _startTime).total_seconds() * _scheduleFixedSteps) / float((_endTime - _startTime).total_seconds());
		_learnRate = _learnInit * pow(_learnDecay, percDone);
	} else if (_scheduleRate == perMaxEpochs) {
		float percDone = epochNum / float(_maxEpochs);
		_learnRate = _learnInit * pow(_learnDecay, percDone);
	}

	logout << "setting learning params: l=" << _learnRate << " m=" << _momRate << " (s=" << _stepsMade << " e=" << epochNum << "), method=" << _scheduleRate << endl;
	writeBuf();
}

}
