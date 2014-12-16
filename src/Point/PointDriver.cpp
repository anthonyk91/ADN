/*
 * BinaryDriver.cpp
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#include <fenv.h>
#include "PointRep.h"
//#include <boost/program_options.hpp>

using namespace adn;
using namespace pointrep;
//namespace po = boost::program_options;

int main(int argc, char* argv[]) {
#ifdef _DEBUG
	feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

	/*// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("train", po::value<string>(), "training data file (csv)")
	    ("minutes", po::value<int>(), "training time (mins)")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
	    cout << desc << "\n";
	    return 1;
	}

	if (vm.count("compression")) {
	    cout << "Compression level was set to "
	 << vm["compression"].as<int>() << ".\n";
	} else {
	    cout << "Compression level was not set.\n";
	}*/

	// set up IO from external sources
    ExternalIO ioHandler;
    if (! ioHandler.readCommandLine(argc, argv)) exit(1);

    try {
		PointDriver driverObject;
		if (! driverObject.verifyActionQueue()) return 1;
		driverObject.runActionQueue();
    } catch (const std::runtime_error& e) {
    	logout << "Error: " << e.what() << ", exiting" << endl;
    	writeBuf();
    	return 1;
    }
    return 0;
}

namespace pointrep {

PointDriver::PointDriver() {
	Params::instance().setBaseValues();

	// set up action objects
	_actionFunctions["setHarness"] 		= ActionFn(1,&DoSetHarness, "[bin|grad]");
	_actionFunctions[ACT_SETBINGAME] 	= ActionFn(1,&DoSetGame, USAGE_SETBINGAME);
	_actionFunctions["storePop"] 		= ActionFn(1,&DoStorePop, "ratio");
	_actionFunctions["loadARFF"] 		= ActionFn(2,&DoLoadARFF, "filename training");
	_actionFunctions["loadMNIST"] 		= ActionFn(3,&DoLoadMNIST, "imgfile labelfile training");
	_actionFunctions["loadCSV"] 		= ActionFn(2,&DoLoadCSV, "imgfile training");
	_actionFunctions["cleardata"] 		= ActionFn(0,&DoClearData);
	_actionFunctions["loadData"] 		= ActionFn(0,&DoLoadData);
}

PointDriver::~PointDriver() {
}

void PointDriver::mkBatches()
{
	_batch.clear();
	if (_batchSize == 0) {
		warn("batch size is 0, setting to 1\n");
		_batchSize = 1;
	}
	for (int i=0; i < _batchSize; ++i) {
		PtInstInfo* bt = new PtInstInfo();
		bt->_harness = _harness;
		_batch.push_back(bt);
	}
	Driver::mkBatches();
}

bool PointDriver::mkHarness(const string& hname) {
    if (hname == "pt") {
    	setHarness(new PtHarness());
    } else if (hname == "grad") {
    	setHarness(new GradHarness());
    } else {
    	logout << "unknown harness type: " << hname << endl;
    	writeBufWarn();
    	return false;
    }
	return true;
}

bool PointDriver::mkWorld(const string& typeName) {
    if (typeName == "mplx") {

    	// check if already mplx world
    	MplxWorld* mWorld = dynamic_cast<MplxWorld*>(_world);
    	if (mWorld) {
    		// just change setting of existing world object
    		mWorld->updateBits();
    	} else {
    		setWorld(new MplxWorld());
    	}
    } else if (typeName == "pplx") {
    	// check if already mplx world
    	ParityplexWorld* pWorld = dynamic_cast<ParityplexWorld*>(_world);
    	if (pWorld) {
    		// just change setting of existing world object
    		pWorld->updateBits();
    	} else {
    		setWorld(new ParityplexWorld());
    	}
    } else if (typeName == "rng") {
    	setWorld(new PointWorld());
    }
    else return false;

    return true;
}


bool DoSetHarness(Driver& d, vector<string>& parameters) {
	int typeParam = 1;

    string& typeName = parameters[typeParam];

    return d.mkHarness(typeName);
}

bool DoSetGame(Driver& d, vector<string>& parameters)
{
	// parameters: name type
    int boardParam = 1;
    int sizeParam = 2;
    int parityParam = 3;

    string& typeName = parameters[boardParam];

    return d.mkWorld(typeName);
}



bool DoStorePop(Driver& d, vector<string>& parameters)
{
    // parameters: name type
    int ratioParam = 1;

    float ratio = 0.5;
    if (parameters.size() > ratioParam) ratio = atof(parameters[ratioParam].c_str());

    // get harness
    PtHarness* fHarness = dynamic_cast<PtHarness*>(d._harness);
    if (! fHarness) {
    	logout << "BinFragHarness not present, invalid use";
    	writeBufWarn();
    	return false;
    }

    fHarness->storePop(ratio);
    return true;
}



bool DoLoadARFF(Driver& d, vector<string>& parameters)
{
	d.checkWorld();
    // parameters: name type
    string filename = parameters[1];
    int training = atoi(parameters[2].c_str());

    PointWorld* rw = dynamic_cast<PointWorld*>(d._world);
    if (! rw) {
    	logout << "range world not found\n";
    	writeBufWarn();
    	return false;
    }

    return rw->loadARFF(filename, training);
}
bool DoLoadMNIST(Driver& d, vector<string>& parameters)
{
	d.checkWorld();
    // parameters: name type
    string imgfilename = parameters[1];
    string labelfilename = parameters[2];
    int training = atoi(parameters[3].c_str());

    PointWorld* rw = dynamic_cast<PointWorld*>(d._world);
    if (! rw) {
    	logout << "range world not found\n";
    	writeBufWarn();
    	return false;
    }

    return rw->loadMNIST(imgfilename, labelfilename, training);
}
bool DoLoadCSV(Driver& d, vector<string>& parameters)
{
	d.checkWorld();

    // parameters: name type
    string filename = parameters[1];
    int training = atoi(parameters[2].c_str());

    PointWorld* rw = dynamic_cast<PointWorld*>(d._world);
    if (! rw) {
    	logout << "range world not found\n";
    	writeBufWarn();
    	return false;
    }

    return rw->loadCSV(filename, training);
}

bool DoClearData(Driver& d, vector<string>& parameters)
{
	d.checkWorld();
    // parameters: name type

    PointWorld* rw = dynamic_cast<PointWorld*>(d._world);
    if (! rw) {
    	logout << "range world not found\n";
    	writeBufWarn();
    	return false;
    }

    rw->clearData();
    return true;
}

bool DoLoadData(Driver& d, vector<string>& parameters)
{
	d.checkWorld();
	// read from specified training files in multi map

    // parameters: name type
    string imgfilename = parameters[1];
    string labelfilename = parameters[2];
    int training = atoi(parameters[3].c_str());

    PointWorld* rw = dynamic_cast<PointWorld*>(d._world);
    if (! rw) {
    	logout << "range world not found\n";
    	writeBufWarn();
    	return false;
    }

    return rw->loadMNIST(imgfilename, labelfilename, training);
}

} // namespace
