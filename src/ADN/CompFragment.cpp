	// first find components that match the given caller instance
#include "ADN.h"
#include "Combinatrix.h"


namespace adn {

using namespace logger;

// note removed setting properties in constructor
CompFragment::CompFragment(const vector<Fragment*>& elements,
	Harness* parent, float biasVal, bool linkChildren) //float associationValue)
	: Fragment(parent, biasVal), _nextKey(0) //, _compProperties(properties)
{

	// create association links to this composite for each element
	vector<Fragment*>::const_iterator iter;
	int maxChildDepth = 0;
	_foreach(elements, iter) {
		Fragment* thisFrag = *iter;

		// copy elements to child map
		_childMap[_nextKey++] = thisFrag;
		if (linkChildren) thisFrag->addParent(this);

		//thisFrag->setAssociation(this, associationValue);
		maxChildDepth = _max(maxChildDepth, thisFrag->_linkDepth);
	}
	_linkDepth = maxChildDepth + 1;
	_storedFrag = false;
	
}

// create with pre-defined map
CompFragment::CompFragment(const FragMap& elements, Harness* parent, float biasVal, bool linkChildren)
	: Fragment(parent, biasVal), _childMap(elements)
{
	// create association links to this composite for each element
	FragMap::const_iterator iter;
	int maxChildDepth = 0;
	_nextKey = 0;
	_foreach(elements, iter) {
		Fragment* thisFrag = iter->second;
		if (linkChildren) thisFrag->addParent(this);
		maxChildDepth = _max(maxChildDepth, thisFrag->_linkDepth);
		if (iter->first > _nextKey) _nextKey = iter->first;
	}
	++_nextKey; // set to value after largest given index
	_linkDepth = maxChildDepth + 1;
	_storedFrag = false;
}

CompFragment::CompFragment(const CompFragment& f)
	: Fragment(f)
{
	warn("CompFragment copy const used\n");
	duplicateProperties(f._compProperties);
	
	_childMap = f._childMap;
	_nextKey = f._nextKey;

	// duplicate associations in child elements
	int maxChildDepth = 0;
	FragMap::iterator cIter;
	_foreach(_childMap, cIter) {
		Fragment* thisFrag = cIter->second;
		thisFrag->addParent(this);
		//thisFrag->setAssociation(this, thisFrag->getAssociation(const_cast<CompFragment*>(&f))); // need non-const so can do table lookup
		maxChildDepth = _max(maxChildDepth, thisFrag->_linkDepth);
	}
	_linkDepth = maxChildDepth + 1;
	_storedFrag = false;
}

CompFragment::~CompFragment()
{
	releaseProperties();	
	releaseChildFragments();
}

void CompFragment::linkChildren()
{
	FragMap::iterator cIter;
	_foreach(_childMap, cIter) {
		Fragment* thisFrag = cIter->second;
		thisFrag->addParent(this);
	}
}

CompFragment& CompFragment::operator=(const CompFragment& f)
{
	if (&f == this) {
		logger::warn("attempted operator= on self (CompFragment)\n");
		return *this;
	}
	Fragment::operator=(f);
	
	releaseProperties();	
	
	duplicateProperties(f._compProperties);

	releaseChildFragments();
	
	_childMap = f._childMap;
	_nextKey = f._nextKey;
	
	// record as parent for each new element
	int maxChildDepth = 0;
	FragMap::iterator cIter;
	_foreach(_childMap, cIter) {
		Fragment* thisFrag = cIter->second;
		thisFrag->addParent(this);
		//thisFrag->setAssociation(this, thisFrag->getAssociation(const_cast<CompFragment*>(&f))); // need non-const so can do table lookup
		maxChildDepth = _max(maxChildDepth, thisFrag->_linkDepth);
	}
	_linkDepth = maxChildDepth + 1;

	return *this;
}

bool CompFragment::operator==(const Fragment& f) {
	error("CompFragment::operator==(const Fragment& f) called, unexpected");
	return false;
}

bool CompFragment::operator==(const CompFragment& frag)
{
	//warn("CompFragment::operator==, shouldn't be used?\n");

	// test if component elements are the same
	unsigned int numEls = _childMap.size();
	if (frag._childMap.size() != numEls) return false;
	
	if (_parHarness->_keepChildOrder) {
		// only check if elements are the same and in the same positions (keys)
		FragMap::const_iterator cIter;
		_foreach(_childMap, cIter) {
			int thisKey = cIter->first;
			Fragment* thisFrag = cIter->second;
			FragMap::const_iterator fIter
				= frag._childMap.find(thisKey);
			if (fIter == frag._childMap.end() ||
					thisFrag != fIter->second) return false;
		}
	} else {
		// find count of each child (ptr) and compare
		map<Fragment*, int> thisCounts, otherCounts;
		FragMap::const_iterator cIter, oIter;
		_foreach(_childMap, cIter) thisCounts[cIter->second]++;
		_foreach(frag._childMap, cIter) otherCounts[cIter->second]++;

		map<Fragment*, int>::iterator mIter;
		_foreach(thisCounts, mIter) if (otherCounts[mIter->first] != mIter->second) return false;
	}
	
	// test if properties are the same.  look at each combination of pairs
	// of properties
	if (_compProperties.empty() && frag._compProperties.empty()) return true;

	vector<CompProperty*> propertyList(_compProperties.begin(), _compProperties.end());
	unsigned int numProperties = _compProperties.size();
	if (frag._compProperties.size() != numProperties) return false;

	// populate combination mechanism
	Combinatrix<CompProperty*> cmb(numProperties);
	vector<CompProperty*> otherPropertyList(frag._compProperties.begin(), frag._compProperties.end());
	for(unsigned int idx = 0; idx < numProperties; ++idx) cmb.setCombination(otherPropertyList, idx);
	
	// examine each combination (set) of properties
	Combinatrix<CompProperty*>::exsel_iterator exselIter;
	for(exselIter = cmb.exsel_begin(); exselIter != cmb.exsel_end(); ++exselIter) {
		vector<CompProperty*> selections = *exselIter;
		
		bool possibleMatch = true;

		unsigned int propIdx;
		for (propIdx = 0; propIdx < numProperties; ++propIdx) {
			// check if this pair of properties matches
			if (! (*propertyList[propIdx] == *selections[propIdx])) {
				possibleMatch = false;
				break;
			}
		}
		
		// all property pairs match
		if (possibleMatch) return true; 
	}
	
	// no matching set found
	return false;
}


int CompFragment::addChild(Fragment* newFrag) {
	// add fragment as new child element

	// add to child map
	int usedKey = _nextKey;
	_childMap[_nextKey++] = newFrag;

	// attach parent link
	newFrag->addParent(this);

	if (_parHarness->_showProcessing) {
		logout << "added new fragment " << newFrag << " " << newFrag->_fragID
				<< " to comp " << this << " " << _fragID << " at pos " << usedKey << endl;
		writeBuf();
	}
	return usedKey;
}

double CompFragment::getChildWeight(int idx)
{
	return _childWeights[idx]._val;
}

NeurParam& CompFragment::getChildWeightParam(int idx)
{
	return _childWeights[idx];
}

void CompFragment::releaseProperties()
{
	// delete any existing property elements
	set<CompProperty*>::iterator iter;
	_foreach(_compProperties, iter) delete *iter;	
	_compProperties.clear();
}

void CompFragment::duplicateProperties(const set<CompProperty*>& properties)
{
	// duplicate properties
	set<CompProperty*>::iterator iter;
	_foreach(properties, iter) {
		CompProperty* copiedProperty = (*iter)->clone(); 
		_compProperties.insert(copiedProperty);
		copiedProperty->setNewParent(*this);
	}
}

unsigned int CompFragment::replaceChild(Fragment* fragA, Fragment* fragB)
{
	//< replace all occurances of B with A
	unsigned int nFound = 0;
	FragMap::iterator cIter;
	_foreach(_childMap, cIter) {
		if (cIter->second == fragB) {
			logout << "compfrag " << this << " " << this->_fragID << " replacing child[" << cIter->first << "] "
					<< fragB << " " << fragB->_fragID << " with " <<  fragA << " " << fragA->_fragID << endl;
			cIter->second = fragA;
			++nFound;
		}
	}
	writeBuf();
	return nFound;
}

void CompFragment::addElement(Fragment* thisFrag)
{
	logout << "comp " << _fragID << " adding element (child?) " << thisFrag->_fragID << endl;
	writeBuf();

	thisFrag->addParent(this);

	int thisLinkDepth = thisFrag->_linkDepth;
	if (thisLinkDepth >= _linkDepth) _linkDepth = thisLinkDepth + 1;

	_childMap[_nextKey++] = thisFrag;
}

unsigned int CompFragment::removeChild(Fragment* frag)
{
	//< remove all occurances of frag
	bool search = true;
	unsigned int found = 0;
	while(search) {
		search = false;
		FragMap::iterator cIter;
		_foreach(_childMap, cIter) {
			if (cIter->second == frag) {
				removeElement(cIter->first, true);
				++found;
				search = true;
				break;
			}
		}
	}
	return found;
}


void CompFragment::removeElement(int key, bool removeProperties)
{

	// find element
	FragMap::iterator oIter
		= _childMap.find(key);
	if (oIter == _childMap.end()) {
		warn("removeElement: key %d not found\n", key);
		return;
	}
	Fragment* thisFrag = oIter->second;
	thisFrag->releaseParent(this, false);

	// remove
	_childMap.erase(oIter);

	// re-evaluate link depth
	_linkDepth = 0;
	_foreach(_childMap, oIter) {
		thisFrag = oIter->second;

		int fragDepth = thisFrag->_linkDepth;
		if (fragDepth >= _linkDepth) _linkDepth = fragDepth;
	}
	_linkDepth = _linkDepth + 1;

	// remove referencing properties (if needed)
	if (removeProperties) {
	    set<CompProperty*>::iterator pIter;
	    for(pIter = _compProperties.begin(); pIter != _compProperties.end();) {
	    	if ((*pIter)->matchesKey(key)) {
	    		delete *pIter;
	    		set<CompProperty*>::iterator cpyIter = pIter;
	    		++pIter;
	    		_compProperties.erase(cpyIter);
	    	} else ++pIter;
	    }
	}
}

void CompFragment::releaseChildFragments()
{
	FragMap::iterator cIter;
	_foreach(_childMap, cIter) {
		Fragment* childEl = cIter->second;
		childEl->releaseParent(this, true);
	}
	_linkDepth = 0;
	_childMap.clear();
}


void CompFragment::copyChildFragments(const FragMap& newEls)
{
	FragMap::const_iterator cIter;
	_foreach(newEls, cIter) addElement(cIter->second);
}


CompInstance* CompFragment::createInstance(const tr1::unordered_map<int,AsscInstance*>& substs, bool removeObj)
{
	warn("line removed\n");
	return NULL;
	//return static_cast<InstHarness*>(_parHarness)->getCompInstance(substs, *this, removeObj);
}


unsigned int CompFragment::numElements()
{ return _childMap.size(); }

void CompFragment::getChildSet(set<Fragment*>& recordSet)
{
	// add self to set
	pair<set<Fragment*>::iterator, bool> result = recordSet.insert(this);
	if (result.second == true) {
		// new call, add sets of children
		FragMap::iterator elIter;
		_foreach(_childMap, elIter) elIter->second->getChildSet(recordSet);
	}	
}


void CompFragment::getChildSet(set<const Fragment*>& recordSet) const
{
	// add self to set
	pair<set<const Fragment*>::iterator, bool> result = recordSet.insert(this);
	if (result.second == true) {
		// new call, add sets of children
		FragMap::const_iterator elIter;
		_foreach(_childMap, elIter) elIter->second->getChildSet(recordSet);
	}	
}

void CompFragment::getChildList(list<Fragment*>& recordList)
{
	// add self to set
	recordList.push_back(this);

	// add children
	FragMap::iterator elIter;
	_foreach(_childMap, elIter) elIter->second->getChildList(recordList);
}


void CompFragment::getChildList(list<const Fragment*>& recordList) const
{
	// add self to set
	recordList.push_back(this);

	// add children
	FragMap::const_iterator elIter;
	_foreach(_childMap, elIter) elIter->second->getChildList(recordList);
}

void CompFragment::getAtomElements(list<Fragment*>& atomSet) const
{
	// recurse and add all atom elements to list.  includes duplicates if atom
	// appears in multiple branches of the tree
	FragMap::const_iterator elIter;
	_foreach(_childMap, elIter) {
		Fragment* thisFrag = elIter->second;
		if (thisFrag->isComposite())
			static_cast<CompFragment*>(thisFrag)->getAtomElements(atomSet);
		else atomSet.push_back(thisFrag);
	}
}

const FragMap& CompFragment::getComponents() const
{ return _childMap; }

Fragment* CompFragment::getComponent(unsigned int key) const
{
	//_Assert(index < _compElements.size(), ("invalid index in call to CompFragment::getChild, %d\n", index));
	FragMap::const_iterator f = _childMap.find(key);
	if (f == _childMap.end()) return NULL;
	else return f->second;
}

const set<CompProperty*>& CompFragment::getCompProperties() const {
	return _compProperties;
}


void CompFragment::display(ostream& output) const
{
	output << "[";
	
	Fragment::display(output);
	
	// display properties of this composite
	set<CompProperty*>::iterator pIter;
	_foreach(_compProperties, pIter) {
		output << "(";
		(*pIter)->display(output);
		output << ")";
	}
	
	output << " ";
	
	// display each element, with identifying number
	bool first = true;
	FragMap::const_iterator elIter;
	_foreach(_childMap, elIter) {
		if (first) first = false;
		else output << ",";
		
		output << elIter->first;
		if (elIter->second->isComposite()) output << "c:";
		else output << "a:";
		
		output << elIter->second->_fragID;
	}
	
	output << "]";
}

void CompFragment::displayRecursive(ostream& output) const
{
	// show this element
	display(output);
	
	// recurse through child elements
	bool first = true;
	FragMap::const_iterator elIter;
	_foreach(_childMap, elIter) {
		if (first) first = false;
		else output << ",";
		
		Fragment* thisFrag = elIter->second;
		output << thisFrag << "=";
		CompFragment* cFrag = dynamic_cast<CompFragment*>(thisFrag);
		if (cFrag == NULL) {
			// atom fragment, use normal display call
			thisFrag->display(output);
		}
		else cFrag->displayRecursive(output);
	}
}

void CompFragment::write(ostream& out) const
{
	Fragment::write(out);

	out << " " << _childMap.size();
	FragMap::const_iterator elIter;
	_foreach(_childMap, elIter) {
		out << " " << elIter->first << " " << elIter->second->_fragID;
	}
	out << " " << _childWeights.size();
	tr1::unordered_map<int, NeurParam>::const_iterator wIter;
	_foreach(_childWeights, wIter) {
		out << " " << wIter->first << " " << wIter->second._val << " " << wIter->second._mom;
	}
}
void CompFragment::read(istream& in)
{
	Fragment::read(in);

	_childMap.clear();
	_childWeights.clear();

	int n, maxKey = 0;
	in >> n;
	for (int i=0; i<n; ++i) {
		int idx, id;
		in >> idx >> id;
		Fragment* frag = _parHarness->_idMap[id];
		if (! frag) warn("could not find composite child\n");
		_childMap[idx] = frag;
		frag->addParent(this);
		maxKey = _max(idx, maxKey);
	}
	in >> n;
	for (int i=0; i<n; ++i) {
		int idx;
		double wt, wtmom;
		in >> idx >> wt >> wtmom;
		_childWeights[idx]._val = wt;
		_childWeights[idx]._mom = wtmom;
	}
	_nextKey = maxKey + 1;
}

bool CompFragment::redundantFrag() const
{
	// check if frag doesn't add any information and
	// can be removed, eg single child, attach child to parents
	// instead
	return false; // subclass
}

void CompFragment::addProperty(CompProperty* prop)
{
	// claims ownership of property for deletion purposes
	_compProperties.insert(prop);
}

bool CompFragment::isComposite() const
{ return true; }

void CompFragment::replaceAtomRefs(Fragment* prevAtom, Fragment* newAtom) {
	// replace property references to prev with new
	// do nothing here, subclass if needed
	return;
}


/*** CompProperty methods ***/

CompProperty::~CompProperty() 
{	
}

void CompProperty::setNewParent(CompFragment& newParent)
{
	_parent = &newParent;
}

bool CompProperty::propertyMatches(const CompInstance& inst)
{ return propertyMatches(inst.getComponentInstances()); }

bool CompProperty::propertyMatches(const CompFragment& frag, CompInstance** foundInst)
{
	// check all instances for match
	const vector<AsscInstance*>& fragInstances = frag.getInstances();
	vector<AsscInstance*>::const_iterator iter;
	_foreach(fragInstances, iter) {
		CompInstance* thisInstance = static_cast<CompInstance*>(*iter);
		if (propertyMatches(*thisInstance)) {
			if (foundInst != NULL) *foundInst = thisInstance;
			return true;
		}
	}
	return false;
}

bool CompProperty::referencesComponent(Fragment* component)
{
	set<Fragment*> allComponents;
	_Assert(getComponentsReferenced(allComponents), ("could not get referenced components\n"));
	return allComponents.find(component) != allComponents.end();
}

CompFragment* CompProperty::getParent() const
{
	return _parent;
}



/*** CompInstance methods ***/
CompInstance::CompInstance(const InstMap& substs, Fragment& parentFrag)
: AsscInstance(parentFrag), _componentInstances(substs) 
{
	// set up observer link for all child instances
	InstMap::const_iterator iter;
	_foreach(substs, iter) watch(*(iter->second));
}


CompInstance::~CompInstance()
{
	
}

bool CompInstance::operator==(const CompInstance& compareInsts) const
{
	return matchesSubsts(compareInsts._componentInstances);
}

bool CompInstance::operator<(const CompInstance& compareInsts) const
{
	// important: assume same keys in each

	// make sorted list of keys
	list<int> keyList;
	InstMap::const_iterator iter;
	_foreach(_componentInstances, iter) {
		keyList.push_back(iter->first);
	}
	keyList.sort();

	// check each key in order to determine <
	list<int>::iterator lIter;
	_foreach(keyList, lIter) {
		AsscInstance* a = _componentInstances.find(*lIter)->second;
		AsscInstance* b = compareInsts._componentInstances.find(*lIter)->second;

		if (a == b) continue; // look at next element
		return a < b;
	}
	return false; // reaches here if equal
}

bool CompInstance::matchesSubsts(const InstMap& substs) const
{
	// check if instance list is the same
	unsigned int listSize = substs.size(); 
	if (listSize != _componentInstances.size()) return false;
	
	InstMap::const_iterator iter;
	_foreach(substs, iter) {
		InstMap::const_iterator f
			= _componentInstances.find(iter->first);
		if (f == _componentInstances.end()) return false; // key not found
		if (iter->second != f->second) return false; // fragments for given key don't match
	}
	return true;
}

void CompInstance::setSubsts(const InstMap& substs)
{
	_componentInstances = substs;
}

const InstMap& CompInstance::getComponentInstances() const
{
	return _componentInstances;
}

void CompInstance::getChildAtomInsts(set<AsscInstance*>& atoms) const
{
	InstMap::const_iterator cIter;
	_foreach(_componentInstances, cIter) {
		AsscInstance* thisInst = cIter->second;
		if (thisInst->isCompositeInstance())
			static_cast<CompInstance*>(thisInst)->getChildAtomInsts(atoms);
		else
			atoms.insert(thisInst);
	}
}

void CompInstance::display(ostream& output) const
{
	output << "compInst{";
	AsscInstance::display(output);
	
	InstMap::const_iterator iter;
	_foreach(_componentInstances, iter) {
		output << iter->first << ":";
		iter->second->display(output);
		output << ",";
	}
	output << "}";
}

void CompInstance::reset() {
	_componentInstances.clear();
}

bool CompInstance::isCompositeInstance() const
{
	return true;
}


} // namespace fragments

