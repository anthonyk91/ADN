#include "ADN.h"

namespace adn {
using namespace logger;

Fragment::Fragment(Harness* parent, float biasVal)
: _biasParam(NeurParam(biasVal)), _parHarness(parent)
{
	_linkDepth = 0;
	_fragID = _parHarness->getFragID();
	//_droppedOut = false;
	_statesObs = 0;
	_statesMatch = 0;
	_matchFreq = 0.0;
	_assocNorm = 0.0;
	reset();
}


Fragment::Fragment(const Fragment& f) // copy const
	: _parHarness(f._parHarness)
{
	_biasParam = f._biasParam;
	_linkDepth = f._linkDepth;
	// don't duplicate instantiations or parents
	_fragID = _parHarness->getFragID(); // give new ID
	//_droppedOut = false;
	_statesObs = 0;
	_statesMatch = 0;
	_matchFreq = 0.0;
	_assocNorm = 0.0;
	reset();
}

Fragment::~Fragment()
{
	if (! _instantiations.empty()) {
		logger::warn("deleting assc fragment when instances exist\n");
	}
	//releaseInstances();

	// check if all parents have been removed
	if (! _parents.empty()) {
		logger::warn("deleting assc fragment when parent links exist\n");
	}
}

Fragment& Fragment::operator=(const Fragment& f) // op=
{
	if (&f == this) {
		logger::warn("attempted operator= on self (Fragment)\n");
		return *this;
	}
	//utils::Associating::operator=(f);
	utils::Displaying::operator=(f);
	
	_statesObs = f._statesObs;
	_matchFreq = f._matchFreq;
	_assocNorm = f._assocNorm;
	_linkDepth = f._linkDepth;
	reset();
	
	return *this;
}


void Fragment::clearInstances()
{
	_instantiations.clear();	
	_numInstances = 0;	
	_instTests = 0;
}

void Fragment::releaseInstances()
{
	logger::warn("attempt to release Fragment instances\n");
	
	// release any instantiations
	vector<AsscInstance*>::iterator iter;
	_foreach(_instantiations, iter) delete *iter;
	_instantiations.clear();	
	_numInstances = 0;
	_instTests = 0;
}

/*void Fragment::setDroppedOut(bool d)
{
	_droppedOut = d;
}
bool Fragment::droppedOut() const { return _droppedOut; }*/

unsigned int Fragment::pruneInstances(unsigned int maxInstances)
{
	unsigned int nRemoved = 0;
	
	// remove from front
	vector<AsscInstance*>::iterator iter = _instantiations.begin();
	while (iter != _instantiations.end() && _numInstances > maxInstances) {
		// check if this instance is depended on by others
		AsscInstance* thisInst = *iter;
		if (thisInst->getObservingComposites() > 0) {
			++iter;
			continue;
		}
		
		// remove this element
		iter = _instantiations.erase(iter);
		delete thisInst;
		--_numInstances;
		++nRemoved;
	}
	
	return nRemoved;
}

void Fragment::reset() {
	clearInstances();
}

void Fragment::removeParents()
{
	const vector<CompFragment*>& parentElements = getParents();
	//logout << "removing parents of " << frag << ", " << *frag << endl;
	//writeBuf();

	while (! parentElements.empty()) {
		// let unlinking be done by parent at point of deconstruction,
		// simply loop while more parent objects remaining
		CompFragment* thisParent = parentElements.front();
		//logout << frag << " remove comp " << thisParent << endl;
		//writeBuf();

		if (! thisParent->removeChild(this)) {
			logout << "failure to remove child " << *this << "from parent " << thisParent << "," << *thisParent << endl;
			writeBufWarn();
			break;
		}

	}

	if (! parentElements.empty()) {
		logout << "looping error removing parents of " << this << ", " << *this << endl;
		writeBufWarn();
	}
}


bool Fragment::findActivation(InstInfo& t) const
{
	// thread focused method
	warn("Fragment::findActivation called, not implemented, should be subclassed");
	//_parHarness
	return false;
}


const vector<CompFragment*>& Fragment::getParents() const
{
	return _parents;
}

void Fragment::display(ostream& output) const
{
	// limited display
	output << _fragID << " i:" << _numInstances << ",d:" << _linkDepth
			<< ",so:" << _statesObs << ",mf:" << _matchFreq
			<< ",an:" << _assocNorm;

	// show classifications
	const AssocMap& classMap = getAssociationSet();
	AssocMap::const_iterator mIter;
	output << "<";
	_foreach(classMap, mIter) {
		mIter->first->display(output);
		output << ":";
		output << mIter->second._val;
		output << " ";
	}
	output << ">";

}

void Fragment::write(ostream& out) const
{
	// write params
	out << _fragID << " " << _linkDepth << " " << _statesObs << " " << _matchFreq << " " << _assocNorm;

	// write out-assocs
	const AssocMap& classMap = getAssociationSet();
	out << " " << classMap.size();
	AssocMap::const_iterator mIter;
	_foreach(classMap, mIter) {
		out << " ";
		mIter->first->display(out);
		out << " ";
		out << mIter->second._val;
		out << " ";
		out << mIter->second._mom;
	}
}

void Fragment::read(istream& in)
{
	// read params
	in >> _fragID >> _linkDepth >> _statesObs >> _matchFreq >> _assocNorm;

	// read out-assocs
	AssocMap& classMap = getModAssociationSet();
	int n;
	in >> n;
	for (int i=0; i<n; ++i) {
		int classID;
		double val, mom;
		in >> classID >> val >> mom;

		classMap[_parHarness->_worldPtr->getClassObj(classID)]._val = val;
		classMap[_parHarness->_worldPtr->getClassObj(classID)]._mom = mom;
	}
}


void Fragment::trackState(bool active, double learnRate)
{
	if (_statesObs < INT_MAX) ++_statesObs;
	if (active && _statesMatch < INT_MAX) ++_statesMatch;

	if (_statesMatch < DIRECTCOUNTS) _matchFreq = _statesMatch / double(_statesObs);
	else _matchFreq += learnRate * ((active ? 1 : 0) - _matchFreq);
}



void Fragment::getChildSet(set<Fragment*>& recordSet)
{
	// attempt to add self to set
	recordSet.insert(this);	
}

void Fragment::getChildSet(set<const Fragment*>& recordSet) const {
	// attempt to add self to set
	recordSet.insert(this);
}

void Fragment::getChildList(list<Fragment*>& recordList)
{
	// attempt to add self to set
	recordList.push_back(this);
}

void Fragment::getChildList(list<const Fragment*>& recordList) const {
	// attempt to add self to set
	recordList.push_back(this);
}

void Fragment::addParent(CompFragment* compFrag)
{
	//logger::trace("fragment %p adding parent %p\n", this, compFrag);
	_parents.push_back(compFrag);
}

void Fragment::releaseParent(CompFragment* compFrag, bool allLinks)
{
	vector<CompFragment*>::iterator iPos;
	for(iPos = _parents.begin(); iPos != _parents.end();) {
		CompFragment* thisComp = *iPos;
		if (thisComp == compFrag) {
			//logout << "fragment " << this << " removing parent " << thisComp << endl;
			//writeBuf();
			iPos = _parents.erase(iPos);
			if (! allLinks) break;			
		} else ++iPos;
	}	
}

void Fragment::clearParents()
{
	_parents.clear();
}

const vector<AsscInstance*>& Fragment::getInstances() const
{ return _instantiations; }

unsigned int Fragment::activeInstances() const // number of instances active
{ return _numInstances; }

void Fragment::addInstance(AsscInstance* inst)
{
	_Assert(&inst->getTemplate() == this, ("attempt to add instance that does not use this fragment as template"));
	_instantiations.push_back(inst);
	++_numInstances;
	warn("line removed\n");
	//++static_cast<InstHarness*>(_parHarness)->_countInstances;
}

bool Fragment::isComposite() const
{ return false; }

Harness* Fragment::getParentHarness() const
{ return _parHarness; }

//void Fragment::setBiasVal(double bias) { _biasVal = bias; }
double Fragment::getBiasVal() const { return _biasParam._val; }

NeurParam& Fragment::biasParam() { return _biasParam; }


bool Fragment::assocNormGThan(const Fragment* a, const Fragment* b)
{
	return a->_assocNorm > b->_assocNorm;
}

double Fragment::findAssocNorm() {
	AssocMap::const_iterator mIter;
	const AssocMap& aMap = getAssociationSet();
	_assocNorm = 0.0;
	_foreach(aMap, mIter) _assocNorm += fabs(mIter->second._val);
	return _assocNorm;
}

double Fragment::calcAssocNorm() {
	AssocMap::const_iterator mIter;
	const AssocMap& aMap = getAssociationSet();
	double assocNorm = 0.0;
	_foreach(aMap, mIter) assocNorm += fabs(mIter->second._val);
	return assocNorm;

}

double Fragment::getAssocNorm() const
{
	return _assocNorm;
}

void Fragment::updateAssocNorm(double val)
{
	//< set to given val if higher
	if (val > _assocNorm) _assocNorm = val;
}
void Fragment::addAssocNorm(double val)
{
	//< set to given val if higher
	_assocNorm += val;
}
void Fragment::setAssocNorm(double val)
{
	_assocNorm = val;
}


void Fragment::doBackprop(InstInfo& t, double delta)
{
	warn("Fragment::doBackprop not implemented, should be subclassed\n");
}



AsscInstance::~AsscInstance()
{
}

Fragment& AsscInstance::getTemplate() const
{
	return *_template;
}

void AsscInstance::setTemplate(Fragment& frag)
{
	_template = &frag;
}

void AsscInstance::display(ostream& output) const
{
	output << "inst[";
	_template->display(output);
	output << "]";
}

int AsscInstance::getObservingComposites() const
{
	return Subject<AsscInstance, CompInstance>::observerCount();
}

bool AsscInstance::isCompositeInstance() const
{
	return false;
}

void AsscInstance::reset() {
	_template = NULL;
}

} //namespace fragments
