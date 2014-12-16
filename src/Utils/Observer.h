/*
  Name: Observer Pattern
  Copyright: Center for the Mind, Australia
  Author: Anthony Knittel
  Date: 22/07/05 17:19
  Description: Class framework for observer pattern, handles registering links between observed objects
*/

#ifndef OBSERVER_H
#define OBSERVER_H

#include <list>

namespace utils {
using namespace std;
	
template <class S, class O>
class Subject;

template <class O, class S>
class Observer
{
    friend class Subject<S, O>;
    public:
        Observer();
        Observer(const Observer& o);
        Observer& operator=(const Observer& o);
        virtual ~Observer();
        
        virtual void watch(Subject<S,O>& subj); // allow assignment/release of const objects
        virtual bool ceaseWatching(Subject<S,O>& subj);
        
        virtual void notification(const Subject<S,O>* notifier); // called when a subject sends a notify signal
        
        virtual const list<Subject<S,O>*>& getSubjects() const;  // returns list of subjects still active

		int subjectCount() const;
    protected:
        virtual bool unlinkSubject(Subject<S,O>& subj);
        list<Subject<S,O>*> _subjects;
    
};

template<class S, class O>
class Subject
{
    friend class Observer<O, S>;
    public:
        Subject();
        Subject(const Subject& s);
        Subject& operator=(const Subject& s);
        virtual ~Subject();
        
        virtual void notify() const;

        virtual const list<Observer<O,S>*>& getObservers() const;  // returns list of observers still active
		int observerCount() const;
    protected:
        virtual bool unlinkObserver(Observer<O, S>& obs);
        virtual void linkWith(Observer<O, S>& obs);
        
        mutable list<Observer<O, S>*> _observers;
    
};

class GenSubject;
class GenObserver : public Observer<GenObserver, GenSubject> {};
class GenSubject : public Subject<GenSubject, GenObserver> {};

} // namespace utils

#include "Observer.tem"

#endif
