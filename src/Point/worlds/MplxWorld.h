/*
 * MplxWorld.h
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#ifndef MPLXWORLD_H_
#define MPLXWORLD_H_

namespace pointrep {

#define NOTDEFINED -1
class MplxWorld : public PointWorld {
public:
	MplxWorld();
	virtual ~MplxWorld();

	virtual void updateBits();
	void setDataBits(int);

	virtual int remainingInsts(int set) const;
	virtual void start(int set);

protected:
	virtual int evaluateAns(PtInstInfo& t) const;
	virtual int evaluateAns(const vector<FValue>& bitString) const;

    virtual void createInstance(InstInfo& t, int set);

    int _mplxSize;
    int _addressBits;
	int _dataBits;
	vector<const Associating*> _classes;
};


}

#endif /* MPLXWORLD_H_ */
