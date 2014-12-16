/*
 * PointWorld.h
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#ifndef POINTWORLD_H_
#define POINTWORLD_H_

// any binary/ternary rule can be tested against a binary world
#include "PointRep.h"

namespace pointrep {

class PointClass;

#define DONTCARE -1
class PointWorld :  public World {
public:
	PointWorld();
	virtual ~PointWorld();

    virtual void reset(); 				//< reset world state, end game
	virtual const string& getStateString(InstInfo& t) const; // returns binary string

    virtual pair<Associating*, Associating*> getBinaryClasses() const; //< returns the two classes used for off and on values

	bool loadARFF(string filename, int set);
	bool loadCSV(string filename, int set);
	bool loadMNIST(string imgfile, string labelfile, int set);
	void clearData();

	virtual void start(int set);

    virtual void createInstance(InstInfo& t, int set);
    virtual int remainingInsts(int set) const;

	virtual Associating* getClassObj(int val) const;
protected:

	bool readARFF(string filename, vector<vector<FValue> >& features, vector<int>& classes, bool takeClass);
	bool readCSV(string filename, vector<vector<FValue> >& features, vector<int>& classes);
    bool addSet(vector<vector<FValue> >& features, vector<int>& classes, int set, bool global = false, double* givenmean = NULL);


	unsigned int _nClasses;

	mutable string _displayCache; // to fit with existing methods

	mutable std::tr1::unordered_map<int, PointClass*> _classObjs;

	mutable pair<Associating*, Associating*> _binClasses;

    // variables for setting up sampling routines
    int _maxClassSize;

    std::tr1::unordered_map<int, vector<int> > _classMap;
	vector<vector<FValue> > _trainingFt;
	vector<const Associating*> _trainingClasses;
	vector<vector<FValue> > _testFt;
	vector<const Associating*> _testClasses;
	vector<vector<FValue> > _predFt;

	int _normClasses;
	int _normData;
	int _smallClassMultiplier;
	int _epochSampleSize;
	unsigned int _trainIdx;
	unsigned int _testIdx;
	unsigned int _predIdx;
	vector<int> _trainingOrder;

	vector<double> _means;
	vector<double> _sds;
	double _globmean;
	double _globsd;
};

class PointClass : public Associating
{
public:
	PointClass(int value);
	virtual ~PointClass();
	int getValue() const;

	virtual void display(ostream&) const;
protected:
	int _value;
};

#define READFAIL 1000

bool readLabels(string filename, vector<int>& labels);
bool readImages(string filename, vector<vector<FValue> >& images);
int readI32(istream& istr);

}

#endif
