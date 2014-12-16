/* PtRep.h
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#ifndef PTREP_H_
#define PTREP_H_

#include "ADN.h"

/* PtRep overview header file, to include all relevant files
 * in the module
 */

using namespace adn;

namespace pointrep {

#define IKEY_ATOMSYMBOLS "atomSymbols"
#define FKEY_IMMATCHTHRESH "imMatchThreshold"
#define FKEY_ACTIVEBUFFER "activationBuffer"

#define IKEY_MPLXSIZE "mplxSize"
#define IKEY_PARITYSIZE "paritySize"


union FValue {
	int iVal;
	float fVal;
};

enum {
	iClass = 1,
	iSeq = 2,
	fSeq = 3
};

#define RANDOMALPHA 0.01
#define FASTLR 0.2

class PointRep {
public:
	PointRep();
	virtual ~PointRep();

	static PointRep& instance();

	int _atomSymbols;

	double _matchThreshold;
	double _activeBuffer;

	static PointRep* _brInstance;

	vector<int> _featureTypes;
	vector<pair<FValue, FValue> > _featureRanges;
	vector<int> _featureClasses;
    vector<vector<string> > _classNames;
};

const int MISSINGCLASS = -1;
const int dontCare = 2;
const int invalidBit = 3;

class GradHarness;

int bin2Int(const std::vector<FValue>& bitString, int startIdx, int length);

struct PtInstInfo : InstInfo {
	vector<FValue> _features;
};

}

#include "PointWorld.h"
#include "PointDriver.h"
#include "worlds/MplxWorld.h"
#include "worlds/ParityplexWorld.h"

#include "PtHarness.h"
#include "PtFragment.h"
#include "PtCompFragment.h"

#include "GradFragment.h"
#include "GradCompFragment.h"
#include "GradHarness.h"


#endif
