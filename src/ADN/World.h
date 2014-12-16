#ifndef WORLD_H
#define WORLD_H

/* Generic world abstract class.  Defines a template that can be accessed
 * from harness and game objects, that control different actors that read
 * and influence the world.
 *
 */

// base classes within this module
#include "Associating.h"
#include "Logging.h"

#include <list>
#include <vector>
#include <tr1/unordered_set>

// place all definitions in module namespace
namespace adn {

using namespace std;
using namespace utils;   // access to functions in 'utils' module
using namespace logger;  // access to functions in 'logger' module

enum {
	testSet = 0,
	trainSet = 1,
	predSet = 2
};

/* World class, mostly a template to allow standard function calls from various
 * actors to observe or act on the world.  The manner in which subclassed players
 * and worlds interact is defined in the subclasses.
 */
class World
{
	public:
        World();
		virtual ~World();

        // internal methods
        virtual void reset(); 				//< reset world state, end game
		int getStateID() const;				//< get ID of current state, increments with each move

		virtual const string& getStateString() const; //< string representation of state

        virtual void createInstance(InstInfo& t, int set); //< called to generate instance of world, eg for new game
        virtual int remainingInsts(int set) const;

        virtual pair<Associating*, Associating*> getBinaryClasses() const; //< returns the two classes used for off and on values
        bool getBinaryClass() const;

    	mutable vector<const Associating*> _allClasses;
    	tr1::unordered_map<const Associating*, double> _classDist;

    	virtual void start(int set);

    	unsigned int _epochSize;
    	unsigned int _testSize;
    	unsigned int _predSize;

    	virtual Associating* getClassObj(int val) const;

	protected:
        // internal values
        bool 		_binaryClass; // uses two output classes

};

} // namespace gameplaying

#endif // WORLD_H
