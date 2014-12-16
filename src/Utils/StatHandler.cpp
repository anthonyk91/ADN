#include "StatHandler.h"
#include "macros.h"

namespace utils
{

StatHandler::StatHandler()
{
}

StatHandler::~StatHandler()
{
}

void StatHandler::outputStats(int iteration)
{
    // call output routine for each handler
    for (tr1::unordered_map<int, StatCollector*>::iterator iter = _collectors.begin();
    	iter != _collectors.end(); ++iter) {
        (*iter).second->doOutputStats(iteration);   
    }
}

void StatHandler::addCollector(StatCollector* c, int tag)
{
    _collectors[tag] = c;   
}

StatCollector* StatHandler::getCollector(int tag)
{
	tr1::unordered_map<int, StatCollector*>::iterator iter
    	= _collectors.find(tag);
    if (iter == _collectors.end()) return NULL;
    else return (*iter).second;
}

void StatHandler::clearCollectors()
{
    _collectors.clear();    
}




StatCollector::StatCollector()
{
}

StatCollector::~StatCollector()
{
}


}
