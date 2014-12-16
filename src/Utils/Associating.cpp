#include "Associating.h"
//#include "logger.h"
#include "macros.h"

namespace utils
{

Associating::Associating(double baseThresh, bool symm)
: _baseThreshold(baseThresh), _symmetrical(symm),
		  _assocUpdates(1)
{}

Associating::~Associating()
{
	// remove links with any in-bound associated elements
	//if (_symmetrical) for(iter = _asscMap.begin(); iter != _asscMap.end(); ++iter) 
		/*if (iter->second > _baseThreshold)*/ //iter->first->removeAssociation(this);

	for(set<const Associating*>::iterator iter = _inAsscs.begin(); iter != _inAsscs.end(); ++iter) {
		//logger::debug("checking in-associations- calling %p to remove out-assc to %p\n", iter->first, this);
		/*if (iter->second > _baseThreshold)*/
		(*iter)->removeAssociation(this);
	}
}

void Associating::setAssociation(const Associating* a, double v)
{
	_outAsscMap[a]._val = v;
	
	// set in-map on a
	//a->_inAsscMap[this] = v;
	a->_inAsscs.insert(this);
	
	if (_symmetrical) {
		a->_outAsscMap[this]._val = v;
		_inAsscs.insert(a);
	}
}

double Associating::getAssociation(const Associating* a, bool* found)
{
	AssocMap::const_iterator fIter = _outAsscMap.find(a);
	if (fIter == _outAsscMap.end()) {
		if (found) *found = false;
		return ASSC_MIN;
	}
	
	if (found) *found = true;
	return fIter->second._val;
}

NeurParam& Associating::getAssocParam(const Associating* a) {
	return _outAsscMap[a];
}

double Associating::getMaxAssociation() const
{
	bool first = true;
	double max;
	AssocMap::const_iterator fIter;
	_foreach(_outAsscMap, fIter) {
		if (first || fIter->second._val > max) {
			first = false;
			max = fIter->second._val;
		}
	}
	return max;
}

void Associating::display(ostream&) const
{

}

const AssocMap& Associating::getAssociationSet() const
{
	return _outAsscMap;
}

AssocMap& Associating::getModAssociationSet()
{
	return _outAsscMap;
}

void Associating::clearAssociation(const Associating* a)
{
	AssocMap::iterator fIter = _outAsscMap.find(a);
	if (fIter != _outAsscMap.end()) _outAsscMap[a] = ASSC_MIN;	
}

void Associating::clearAllAssociations()
{
	_outAsscMap.clear();
	_inAsscs.clear();
}

/*void Associating::clearInAssociation(const Associating* a)
{

	AssocMap::iterator fIter = _inAsscMap.find(a);
	if (fIter != _inAsscMap.end()) _inAsscMap[a] = ASSC_MIN;	
}*/

void Associating::removeAssociation(const Associating* a) const
{
	AssocMap::iterator fIter = _outAsscMap.find(a);
	if (fIter != _outAsscMap.end()) _outAsscMap.erase(fIter);	
}

int Associating::trimAssocs(double cutoff) {
	// remove all below cutoff
	int found = 0;
	AssocMap::iterator iter = _outAsscMap.begin();
	while (iter != _outAsscMap.end()) {
		if (iter->second._val < cutoff) {
			iter = _outAsscMap.erase(iter);
			++found;
		}
		else iter++;
	}
	return found;
}
bool Associating::addAssoc(const Associating* a, double val)
{
	AssocMap::iterator fIter = _outAsscMap.find(a);
	if (fIter == _outAsscMap.end()) {
		_outAsscMap[a] = val;
		return true;
	}
	return false;
}


void Associating::getAssociations(set<const Associating*>& returnSet) const
{
	for(AssocMap::const_iterator iter = _outAsscMap.begin(); iter != _outAsscMap.end(); ++iter)
		if (iter->second._val >= _baseThreshold) returnSet.insert(iter->first);
}

void Associating::removeNonAssociations() // remove all under threshold
{

	set<const Associating*>::iterator iter;
	AssocMap::iterator fIter;
	set<const Associating*> tmpAssocs = _inAsscs;
	_inAsscs.clear();
	_foreach(tmpAssocs, iter) {
		const Associating* thisAssc = *iter;
		fIter = thisAssc->_outAsscMap.find(this);
		if (fIter != (*iter)->_outAsscMap.end() &&
				fIter->second._val >= _baseThreshold)
			_inAsscs.insert(thisAssc);
	}
	/*AssocMap tmpMap = _inAsscMap;
	_inAsscMap.clear();
	for(AssocMap::iterator iter = tmpMap.begin(); iter != tmpMap.end(); ++iter)
		if (iter->second >= _baseThreshold) _inAsscMap[iter->first] = iter->second;*/

	AssocMap tmpMap = _outAsscMap;
	_outAsscMap.clear();
	for(AssocMap::iterator iter = tmpMap.begin(); iter != tmpMap.end(); ++iter)
		if (iter->second._val >= _baseThreshold) _outAsscMap[iter->first] = iter->second;
}

int Associating::getAssocUpdates() const
{
	return _assocUpdates;
}
void Associating::incrAssocUpdates() {
	_assocUpdates++;
}


} // namespace utils
