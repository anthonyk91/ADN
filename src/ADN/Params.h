/*
 * Params.h
 *
 *  Created on: 26/05/2014
 *      Author: anthony
 */

#ifndef PARAMS_H_
#define PARAMS_H_

#include "Associating.h"
#include <math.h>
#include <boost/date_time.hpp>

namespace adn {

// parameter labels
#define IKEY_MATCHESPEREVAL "matchesPerEval"
#define FKEY_REPRBETA "reproductionBeta"
#define INT_NEURMETHOD "neurMethod"
#define IKEY_NUMTHREADS "numThreads"
#define IKEY_TRAINTIME "trainTime"
#define IKEY_MAXEPOCHS "maxEpochs"

enum schedule {
	perStep = 1,
	perEpoch = 2,
	perEpochMomentum = 3,
	perTime = 4,
	perMaxEpochs = 5
};

enum neurMethods {
	neurtanh = 1,
	neurrelu = 2,
	neurlog = 3,
	neurlin = 4,
	neurretan = 5
};

/*enum {
	dropoutFrags = 1,
	dropoutNorm = 2,
	dropoutObs = 4
};*/

// object for handling hyper-parameters
class Params {
public:
	Params();
	virtual ~Params();

	// probabilistic elements
    float _reproductionBeta;

	// singleton
    static Params& instance();
	static Params* _pinstance; // singleton pointer

	virtual void setBaseValues();

    /*int _dropoutFrags;
	float _dropoutObsRate;
	float _dropoutCompRate;*/

	static int _neurMethod;
	static double _MAXTARGET;
	static double _MINTARGET;
	static double _NEUTRVAL;

    // for scheduled learning/momentum
    int _scheduleRate;
    double _learnInit;
    double _learnDecay;
    double _momInit;
    int _fastEpochs;
    double _momFinal;
    int _updateSteps;
    int _scheduleFixedSteps;
    int _trainTime;

    int _maxEpochs;
    double _learnRate;
    double _momRate; // momentum for updates
    double _wtDecay;

    int _nThreads;

	void resetSchedule();
	void updateSchedule(int epochSize, int steps);
	static void setupNeurMethod();
	int _stepsMade;

	boost::posix_time::ptime _startTime;
	boost::posix_time::ptime _endTime;

protected:
	Params(const Params& gp);
	Params& operator=(const Params& gp);

};


inline double NeurFn(double x) {
	switch (Params::_neurMethod) {
	case neurtanh:
		return tanh(x);
		break;
	case neurrelu:
		return _max(0.0, x);
		break;
	case neurlog:
		return (1.0 / (1.0 + exp(-x)));
		break;
	case neurlin:
		return x;
		break;
	case neurretan:
		return _max(0.0, tanh(x));
		break;
	}
	return 0.0;
}
inline double NeurDelta(double x) {
	switch (Params::_neurMethod) {
	case neurtanh:
	case neurretan:
		return 1.0 - x * x;
		break;
	case neurrelu:
	case neurlin:
		return 1.0;
		break;
	case neurlog:
		return x * (1.0 - x);
		break;
	}
	return 1.0;
}

// properties for use with cross-entropy
//#define OutFn(x) ()
#define SMAXHTARGET 1.0
#define SMAXLTARGET 0.0

using namespace utils;

inline void updateWts(double grad, NeurParam& param, double learnRate = Params::_pinstance->_learnRate) {
	// first update momentum value
	param._mom += (grad - param._mom) * Params::_pinstance->_momRate;

	// perform weight decay
	param._val *= Params::_pinstance->_wtDecay;

	// then update weight
	param._val -= learnRate * param._mom;
}


}


#endif /* PARAMS_H_ */
