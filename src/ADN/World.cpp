//#include "world.h"
#include "ADN.h"

namespace adn {

/*** World public methods ****/

World::World()
{
	// constructor definitions
    _epochSize = 0;
    _testSize = 0;
    _predSize = 0;
    _binaryClass = false;
    //_findROC = false;
}


World::~World()
{
}

// reset world state
void World::reset()
{
}

// calls to create new instance.  subclasses will define the
// operation needed here, however calls from subclass to this
// method will allow handling proper checks and settings
void World::createInstance(InstInfo& t, int set)
{
}
int World::remainingInsts(int set) const { return 1; }

void World::start(int set) {}

const string& World::getStateString() const {

}
pair<Associating*, Associating*> World::getBinaryClasses() const
{
	return pair<Associating*, Associating*>(NULL, NULL);
}

bool World::getBinaryClass() const
{
	return _binaryClass;
}

Associating* World::getClassObj(int val) const
{
	return NULL;
}


} // namespace gameplaying
