#ifndef RNGHANDLER_H_
#define RNGHANDLER_H_

// Singleton to handle the use of GSL routines as a random-number generator

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

// set up random number generator access macros
#define _NRand() gsl_rng_get(utils::RNGHandler::getRNG())
#define _NRandMax gsl_rng_max(utils::RNGHandler::getRNG())
#define _fRand() gsl_rng_uniform(utils::RNGHandler::getRNG())
#define _SeedRand(n) gsl_rng_set(utils::RNGHandler::getRNG(), (n))
#define _pRand(n) (gsl_rng_uniform(utils::RNGHandler::getRNG()) < (n))
#define _gRand(sigma) gsl_ran_gaussian(utils::RNGHandler::getRNG(), (sigma))
#define _ugRand() gsl_ran_ugaussian(utils::RNGHandler::getRNG())

namespace utils
{


class RNGHandler
{
public:
	static gsl_rng* getRNG();
	
protected:
	RNGHandler();
	virtual ~RNGHandler();
	gsl_rng* _gRNG;
	static RNGHandler* _singletonRNG;
};

}

#endif /*RNGHANDLER_H_*/
