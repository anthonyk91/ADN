/*
 * ADN.cpp
 *
 *  Created on: 02/06/2014
 *      Author: anthony
 */

#include "ADN.h"

namespace adn {

void InstInfo::reset() {
	// clear activity lists (no state carry-over)
	_activeFragments.clear();
	_activeAtomFragments.clear();
	_activeComposites.clear();

	// just clear values, avoid deleting entries.  remove deleted frags
	// elsewhere (using clearMaps)
	tr1::unordered_map<const Fragment*, FragDetails>::iterator fiter;
	_foreach(_fragDetails, fiter) {
		FragDetails& f = fiter->second;
		f._match = 0;
		f._activation = 0.0;
		tr1::unordered_map<NeurParam*, vector<double> >::iterator pIter;
		_foreach(f._paramChanges, pIter) pIter->second.clear();
		f._newAssocClasses.clear();
		f._newAssocVals.clear();
	}

	// clear outputs
	ClassMap::iterator iter;
	_foreach(_classActivations, iter) iter->second = 0.0;
	_foreach(_outputs, iter) iter->second = 0.0;
	_foreach(_classDeltas, iter) iter->second = 0.0;
	_foreach(_classBiasMods, iter) iter->second = 0.0;

	_newAtoms.clear();
	_newComps.clear();
}

void InstInfo::clearMaps() {
	// need to trim maps, otherwise it will maintain links and
	// grow as new frags are added.  this does lead to some degree
	// of allocation cycling
	_fragDetails.clear();
}

vector<JobThread*> JobThread::_jobThreads;
vector<boost::thread*> JobThread::_threadHandles;

boost::condition_variable JobThread::queue_change;
boost::mutex JobThread::queue_mut;
list<Task> JobThread::job_queue;

Task::Task()
	: _job(jobIdle), _curInst(NULL)
{

}

JobThread::JobThread()
: _busy(true)
{

}

void JobThread::mainLoop() {
	do {
		// wait for job on queue
		{
			boost::mutex::scoped_lock qlock(JobThread::queue_mut);
			while(JobThread::job_queue.empty()) {
				{
					boost::mutex::scoped_lock lock(_stateLock);
					_busy = false;
				}
				_stateChange.notify_all();
				JobThread::queue_change.wait(qlock);
			}
			boost::mutex::scoped_lock lock(_stateLock);
			_busy = true;
			_workingTask = JobThread::job_queue.front();
			JobThread::job_queue.pop_front();
		}
		JobThread::queue_change.notify_all(); // have to notify all to ensure base thread receives message

		// do job
		if (_workingTask._job == jobProcInstance) {
			if (_workingTask._curInst)
				_workingTask._curInst->processInstance(_workingTask);
		} else if (_workingTask._job == jobApplyUpdates) {
			if (_workingTask._curInst)
				_workingTask._curInst->processFrags(_workingTask);
		} else if (_workingTask._job == jobTerminate) {
			return;
		}

	} while(true);
}

void InstInfo::processInstance(Task& d) {
	if (! d._curInst) {
		logout << "no instance given to process" << endl;
		writeBuf();
		return;
	}
	InstInfo& t = *d._curInst;

	boost::mutex::scoped_lock lock(t._writeLock);
	// 	activate
	t._lastGuess = t._harness->getClassification(t);

	//  find backprop
	if (d._training)
		t._harness->postClassification(t);

	t._correct = (t._currentClass == t._lastGuess);
}

void InstInfo::processFrags(Task& d) {
	if (! d._curInst) {
		logout << "no instance given to process" << endl;
		writeBuf();
		return;
	}

	// apply fragment weight updates
	int nAtoms = d._atoms->size();
	int atomMax = _min(d._high, nAtoms);
	int compMin = _max(d._low, nAtoms);
	int i;
	if (d._low < nAtoms) {
		_Assert(atomMax <= nAtoms, ("unexpected, index out of atom range\n"));
		for (i=d._low; i < atomMax; ++i) fragApplyChanges(d, d._atoms->at(i));
	}

	if (d._high > nAtoms) {
		_Assert(d._high - nAtoms <= d._comps->size(), ("unexpected, index out of comp range. (%d comps)\n", d._comps->size()));
		for (i=compMin; i < d._high; ++i) fragApplyChanges(d, d._comps->at(i - nAtoms));
	}
}

void InstInfo::fragApplyChanges(Task& d, Fragment* frag) {
	tr1::unordered_map<const Fragment*, FragDetails>::iterator fIter;
	tr1::unordered_map<NeurParam*, vector<double> >::iterator pIter;

	tr1::unordered_map<NeurParam*, vector<double> > pChanges;

	vector<InstInfo*>::iterator tIter;
	for (int n=0; n < d._batchSize; n++) {
		InstInfo& t = *d._batch->at(n);
		// get param updates for this frag
		if ((fIter = t._fragDetails.find(frag)) == t._fragDetails.end()) return;
		FragDetails& f = fIter->second;

		// record all delta values for each parameter
		_foreach(f._paramChanges, pIter) {
			pChanges[pIter->first].insert(pChanges[pIter->first].end(), pIter->second.begin(), pIter->second.end());
		}

		// add new sparse weights
		for (int i=0; i<f._newAssocClasses.size(); ++i) {
			const Associating* cls = f._newAssocClasses[i];
			if (frag->addAssoc(cls)) {
				// gets initialised as zero, then update weight
				d._curInst->_harness->incrAssocs();
			}
			// try to update weight (add to change list)
			AssocMap& m = frag->getModAssociationSet();
			AssocMap::iterator fIter;
			if ((fIter = m.find(cls)) != m.end()) {
				pChanges[&(fIter->second)].push_back(f._newAssocVals[i]);
			}
		}

		// possibly calculate stats, such as p(correct | fragactive), or p(correct & fragactive)
	}

	// apply each param change using average
	vector<double>::iterator lIter;
	_foreach(pChanges, pIter) {
		NeurParam& param = *pIter->first;

		double avgGrad = 0.0;
		double count = 1.0;
		_foreach(pIter->second, lIter) {
			avgGrad += (*lIter - avgGrad) / count;
			count += 1.0;
		}
		updateWts(avgGrad, param);
	}

}

void JobThread::waitQueueFinished()
{
	// loop until queue empty
	boost::mutex::scoped_lock lock(JobThread::queue_mut);
	while(! JobThread::job_queue.empty()) {
		JobThread::queue_change.wait(lock);
	}
}

void JobThread::waitAllIdle() {
	if (Params::_pinstance->_nThreads < 1) return;

	// assume threads are no longer going from idle to busy,
	// check each in turn and wait if necessary
	for (int i=0; i<Params::_pinstance->_nThreads; ++i) {
		boost::mutex::scoped_lock jlock(_jobThreads[i]->_stateLock);
		while(_jobThreads[i]->_busy) {
			_jobThreads[i]->_stateChange.wait(jlock);
		}
	}
}

void JobThread::mkThreads() {
	_jobThreads.clear();
	for (int i=0; i<Params::_pinstance->_nThreads; ++i) {
		JobThread* j = new JobThread();
		_jobThreads.push_back(j);

		// launch thread using ref
		_threadHandles.push_back(new boost::thread( boost::bind(&JobThread::mainLoop, j)) );
	}
}

void JobThread::clearThreads() {
	waitAllIdle();

	// log terminate jobs to end thread loops
	vector<JobThread*>::iterator tIter;
	_foreach(_jobThreads, tIter) {
		Task term;
		term._job = jobTerminate;
		boost::mutex::scoped_lock lock(JobThread::queue_mut);
		JobThread::job_queue.push_back(term);
		JobThread::queue_change.notify_one();
	}

	vector<boost::thread*>::iterator hIter;
	_foreach(_threadHandles, hIter) {
		(*hIter)->join();
		delete *hIter;
	}
	_threadHandles.clear();

	_foreach(_jobThreads, tIter) delete *tIter;
	_jobThreads.clear();
}


} // namespace
