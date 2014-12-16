/*
 * ParityplexWorld.h
 *
 *  Created on: Nov 4, 2010
 *      Author: anthony
 */

#ifndef PARITYPLEXWORLD_H_
#define PARITYPLEXWORLD_H_

namespace pointrep {

class ParityplexWorld : public PointWorld {
public:
	ParityplexWorld();
	virtual ~ParityplexWorld();

	virtual void updateBits();

	virtual int remainingInsts(int set) const;
	virtual void start(int set);

protected:
	virtual int evaluateAns(PtInstInfo& t) const;
	virtual int evaluateAns(const vector<FValue>& bitString) const;

	virtual void createInstance(InstInfo& t, int set);

	int _addressBits;
	int _dataBits;
    int _paritySize;
    int _mplxSize;

    static int getParity(const vector<FValue>& bitstring);
};

}

#endif /* PARITYPLEXWORLD_H_ */
