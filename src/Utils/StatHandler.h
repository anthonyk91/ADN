#ifndef STATHANDLER_H_
#define STATHANDLER_H_

/* object to manage record tracking objects.  each record object will be managed by this handler, which will
 * be the point of call to trigger output requests for each record object
 */
#include "Observer.h"
//#include <map>
#include <tr1/unordered_map>
/*namespace std {
	using namespace tr1;
}*/
/*#include <ext/hash_map>
namespace std { using namespace __gnu_cxx; }
#define unordered_map std::hash_map
*/

namespace utils
{
    
class StatCollector;

class StatHandler : public Observer<StatHandler, StatCollector>
{
public:
	StatHandler();
	virtual ~StatHandler();

    void outputStats(int iteration); // calls output routines for each collector
    void addCollector(StatCollector* c, int tag = 0);
    
    StatCollector* getCollector(int tag);
    void clearCollectors();
    
protected:
    tr1::unordered_map<int, StatCollector*> _collectors;
};

class StatCollector : public Subject<StatCollector, StatHandler>
{
public:
    StatCollector();
    virtual ~StatCollector();
    
    virtual void doOutputStats(int iteration) = 0;
        
protected:

};


}

#endif /*STATHANDLER_H_*/
