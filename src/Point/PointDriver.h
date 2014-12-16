/*
 * PointDriver.h
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#ifndef PTDRIVER_H_
#define PTDRIVER_H_

#include "PointRep.h"

namespace pointrep {
using namespace adn;

#define ACT_SETBINGAME "setbingame"

#define USAGE_SETBINGAME "(mplx|pplx|rng) [size]"

class PointDriver : public Driver {
public:
	PointDriver();
	virtual ~PointDriver();

	virtual bool mkHarness(const string& hname);
	virtual bool mkWorld(const string& wname);
	virtual void mkBatches();

protected:
    PointRep _binRepObj; // just define singleton here
    //ImGridObj _imGridObj; // just define singleton here
	Params _params; //< create singleton instance


};

bool DoSetHarness(Driver& d, vector<string>& parameters);
bool DoSetGame(Driver& d, vector<string>& parameters);
bool DoStorePop(Driver& d, vector<string>& parameters);
bool DoLoadARFF(Driver& d, vector<string>& parameters);
bool DoLoadMNIST(Driver& d, vector<string>& parameters);
bool DoLoadCSV(Driver& d, vector<string>& parameters);
bool DoClearData(Driver& d, vector<string>& parameters);
bool DoLoadData(Driver& d, vector<string>& parameters);

}

#endif
