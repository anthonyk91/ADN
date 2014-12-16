#ifndef ASSOCIATING_H_
#define ASSOCIATING_H_

#include <set>
//#include <map>
#include <tr1/unordered_map>
/*namespace std {
	using namespace tr1;
}*/
#include <iostream>
#include "macros.h"

/*#include <ext/hash_map>
namespace std { using namespace __gnu_cxx; }
#define unordered_map std::hash_map
*/

namespace utils
{

using namespace std;

#define ASSC_MIN 0.0

struct NeurParam {
	NeurParam() : _val(0.0), _mom(0.0) {}
	NeurParam(double val) : _val(val), _mom(0.0) {}
	double _val;
	double _mom;
};


/*inline void NeurParam::doUpdate(double newVal, double learnRate) {
	// perform update using 1 / N, until lower than learnRate
	double doRate = learnRate;
	if (_upCount < INT_MAX) {
		_upCount++;
		double linRate = 1 / double(_upCount);
		doRate = _max(linRate, learnRate);
	}
	_val += doRate * (newVal - _val);
}*/

inline double operator+(const NeurParam& n, const double& d) {
	return n._val + d;
}
inline double operator+(const double& d, const NeurParam& n) {
	return n._val + d;
}
inline double operator*(const NeurParam& n, const double& d) {
	return n._val * d;
}
inline double operator*(const double& d, const NeurParam& n) {
	return n._val * d;
}
inline double operator-(const NeurParam& n, const double& d) {
	return n._val - d;
}
inline double operator-(const NeurParam& n, const NeurParam& o) {
	return n._val - o._val;
}
inline double operator/(const NeurParam& n, const double& d) {
	return n._val / d;
}
inline bool operator<(const NeurParam& n, const NeurParam& o) {
	return n._val < o._val;
}

class Associating;
typedef std::tr1::unordered_map<const Associating*, NeurParam> AssocMap;


class Associating
{
public:
	Associating(double baseThresh = 0.0, bool symm = false);
	virtual ~Associating();

	void setAssociation(const Associating*, double);
	double getAssociation(const Associating*, bool* found = NULL);
	NeurParam& getAssocParam(const Associating*); //< creates if not found
	void clearAssociation(const Associating* a);
	virtual void clearAllAssociations();
	//void clearInAssociation(const Associating* a);
	void removeAssociation(const Associating*) const;
	double getMaxAssociation() const;
	int trimAssocs(double val); // remove all below cutoff
	
	void getAssociations(set<const Associating*>&) const;
	const AssocMap& getAssociationSet() const;
	AssocMap& getModAssociationSet();
	bool addAssoc(const Associating* a, double val = 0.0);
	
	void removeNonAssociations(); // remove all under threshold
	int getAssocUpdates() const;
	void incrAssocUpdates();
	
	virtual void display(ostream&) const;
protected:
    mutable AssocMap _outAsscMap;
    //mutable unordered_map<const Associating*, double> _inAsscMap;
    mutable set<const Associating*> _inAsscs;
    double _baseThreshold;
    bool _symmetrical;

    int _assocUpdates;
};

}

#endif /*ASSOCIATING_H_*/
