#include "RNGHandler.h"

namespace utils
{

RNGHandler* RNGHandler::_singletonRNG = NULL;

gsl_rng* RNGHandler::getRNG()
{
	// check if instance exists
	if (_singletonRNG == NULL) {
		// create new instance
		_singletonRNG = new RNGHandler();
	}
	
	return _singletonRNG->_gRNG;
}

RNGHandler::RNGHandler()
{
    gsl_rng_env_setup();
    _gRNG = gsl_rng_alloc(gsl_rng_taus2);
}

RNGHandler::~RNGHandler()
{
}

}
