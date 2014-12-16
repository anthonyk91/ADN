#ifndef Combinatrix_H_
#define Combinatrix_H_

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
//#include <map>
#include <tr1/unordered_map>
/*namespace std {
	using namespace tr1;
}*/

/*#include <ext/hash_map>
namespace std { using namespace __gnu_cxx; }
#define unordered_map std::hash_map
*/

#include "Observer.h"
#include "macros.h"

namespace utils
{

template <typename T> class CombSeqIterator; // basic interface
template <typename T> class CombSelIterator; // selection iterator
template <typename T> class CombExSelIterator; // exclusive selection iterator
template <typename T> class CombSortIterator;
template <typename T> class Combinatrix;

/* basic iterator generates selections of elements in table, choosing one
 * element from each entry for each iterator increment */
template <typename T>
class CombSelIterator :  public Observer<CombSelIterator<T>, Combinatrix<T> > {
friend class Combinatrix<T>;
public:
    CombSelIterator() : _valid(false), _end(true), _parent(NULL) {}
    CombSelIterator(const Combinatrix<T>* parent, bool end=false);
    virtual ~CombSelIterator();

    vector<T>& operator*();
    const CombSelIterator<T>& operator++();
    bool operator!=(const CombSelIterator<T>& it) const;
    
    virtual void notification(); // called to invalidate iterator
    bool isValid() const;
    
protected:
    typedef pair<int,int> table_entry;
    typedef vector<table_entry> table_type;
    bool increment(unsigned int incrIndex = 0); // returns false if incremented past end, true otherwise
    
    bool _valid;
    bool _end;
    
    const Combinatrix<T>* _parent;

    unsigned int _numRows; // number of elements in map
    //table_type _tablePositions; // first value is position, second value is size of substitution list

    vector<int> _positions;
    vector<int> _entrySizes;
    
    vector<T> _returnVector;
};

#define VALID_POSITION -1
template <typename T>
class CombExSelIterator : public CombSelIterator<T>
{
public:
    // act as sel iterator, however monitor object used for each and prevent
    // multiple selection of each object (assumes objects repeat between
    // categories)
    CombExSelIterator();
    CombExSelIterator(const Combinatrix<T>* parent, bool end=false);
    virtual ~CombExSelIterator();
    
    const CombSelIterator<T>& operator++();
    

protected:
    int findInvalidPosition();
    
    int _numElems;
    tr1::unordered_map<T,int> _elemMap;
    vector<vector<int> > _elemArray;
    vector<bool> _matchList;

    int findInvalidPosition_prev();
};

// common interface of sequence iterators:
template<typename T>
class CombSeqIterator : public Observer<CombSeqIterator<T>, Combinatrix<T> >{
friend class Combinatrix<T>;
public:
    CombSeqIterator() : _valid(false), _end(true), _parent(NULL) {}
    CombSeqIterator(Combinatrix<T>* parent, bool end=false)
        : _valid(true), _end(end), _parent(parent) {}
    virtual ~CombSeqIterator();
    virtual const typename Combinatrix<T>::table_type& operator*();

    virtual const CombSeqIterator<T>& operator++() = 0;
    virtual bool operator!=(const CombSeqIterator<T>& it) const;

    virtual void notification(); // called to invalidate iterator
    //virtual void increment(); // hack to allow subclasses using op++

protected:
    bool isValid();
    bool isEnd();
    void setEnd(bool end);
    
    bool _valid;
    bool _end;
    
    Combinatrix<T>* _parent;
};


/* sort iterator generates permutations of sorting of elements in the combination table */
template <typename T>
class CombSortIterator : public CombSeqIterator<T>
{
friend class Combinatrix<T>;
public:
    CombSortIterator() {}
    CombSortIterator(Combinatrix<T>* parent, bool initToFirstSequence = true, bool end=false);
    const CombSortIterator<T>& operator++(); // prefix
    bool operator!=(const CombSortIterator<T>& it) const;
    
};

/* random iterator generates random permutations of sorting of elements in 
 * the combination table.  it is not guaranteed to cover each sorting
 * and may repeat sortings. */
template <typename T>
class CombRandIterator : public CombSeqIterator<T>
{
friend class Combinatrix<T>;
public:
    CombRandIterator() {}
    CombRandIterator(Combinatrix<T>* parent, int ttl, bool end);
    const CombRandIterator<T>& operator++(); // prefix
    bool operator!=(const CombRandIterator<T>& it) const;
    
protected:
    int _ttl;
};



template <typename T>
class Combinatrix : public Subject<Combinatrix<T>,CombSelIterator<T> >,
    public Subject<Combinatrix<T>, CombSeqIterator<T> >
{
friend class CombSelIterator<T>;
friend class CombExSelIterator<T>;
friend class CombSeqIterator<T>;
friend class CombSortIterator<T>;
friend class CombRandIterator<T>;
public:
    typedef CombSelIterator<T> sel_iterator;
    typedef CombExSelIterator<T> exsel_iterator;
    typedef CombSortIterator<T> sort_iterator;
    typedef CombRandIterator<T> rand_iterator;
    typedef vector<T> sequence_type;
    typedef vector<sequence_type> table_type;
    
	Combinatrix(unsigned int numCombinations);
	virtual ~Combinatrix();
    
    void setTable(const table_type& inTable);
    void setCombination(const sequence_type&, unsigned int index);       
    //void addCombination(const sequence_type&);       
    long totalSequences(); // number of sequences that will be generated from data set
    long totalSelections(); // number of selection combinations from data set
    
    string display();
    
    sel_iterator sel_begin() const;
    sel_iterator sel_end() const;

    exsel_iterator exsel_begin() const; // exclusive selection (no duplicates)
    exsel_iterator exsel_end() const;
    
    sort_iterator sort_begin(bool initToFirstSequence = true);
    sort_iterator sort_end();

    rand_iterator rand_begin(int nSamples); // how many samples to give it before it says it is at end
    rand_iterator rand_end();

    typename table_type::const_iterator table_begin() const;
    typename table_type::const_iterator table_end() const;
     
    void invalidateIterators();
    
    bool empty() const;
    void clear();
protected:
    table_type _combTable;
};


#include "Combinatrix.tem"

}

#endif /*Combinatrix_H_*/
