// Thread Pool - multithreading, used for training the trees 
// Original code by Cliff Brunk, modified by Daria Sorokina. Yandex Labs, 2010.


//////////////////////////////////////////////////////////////////////
// thread_pool.h: interface for the TThreadPool
//////////////////////////////////////////////////////////////////////


#ifndef _WIN32

#ifndef _TTHREAD_POOL_H_INCLUDED_
#define _TTHREAD_POOL_H_INCLUDED_

#include <pthread.h>


#include <assert.h>
#include <iostream>

#include <stdio.h>



////////////////////////////////////////////////////
// Template linked list class
////////////////////////////////////////////////////

template <class Type> 
class TLinkedList
{
public:

    class TItem
    {
    public:
        Type Data;
        TItem *Next;
        
        TItem(const Type& data, TItem* next = NULL) : Data(data), Next(next) { }
    };
    
    
    class iterator
    {
        friend class TLinkedList<Type>;
        
    private:

        TItem *item;
        
    public:

        iterator()                     : item(NULL) {}
        iterator(TItem *elem)          : item(elem) {}
        iterator(const iterator &iter) : item(iter.item) {}
        
        bool eol() const { return (item == NULL); }

        iterator& operator= (const iterator& i) { item = i.item; return *this; }
        
        const Type& operator()() const { assert(item != NULL); return item->Data; }
        
        iterator& operator++() { if (item) item = item->Next; return *this; }
        iterator operator++(int) { iterator tmp(*this); ++*this; return tmp; }

        Type& operator*() { assert(item != NULL); return item->Data; }
        Type* operator->() { assert(item != NULL); return & item->Data; }

        bool operator==(const iterator& i) const { return (item == i.item); }
        bool operator!=(const iterator& i) const { return (item != i.item); }
        
    };

private:

    TItem *First;
    TItem *Last;
    unsigned int Size;

public:

    TLinkedList()
        : First(NULL),
          Last(NULL),
          Size(0)
    { }

    ~TLinkedList() { clear(); }

    unsigned int size() const { return Size; }
    
    const iterator first() const { return First; }

    TLinkedList<Type>& push_back(const Type &elem)
    {
        TItem* tmp = new TItem(elem);
        
        if (Last) {
            Last->Next = tmp;
        }
        
        Last = tmp;
        
        if (!First) {
            First = tmp;
        }
        
        Size++;
        return *this;
    }


    // return first item and remove it from list
    Type pop_front_and_return()
    {
        assert(First != NULL);
        
        Type ret = First->Data;
        TItem *tmp = First->Next;
        
        delete First;
        
        if (tmp == NULL) {
            Last = NULL;
        }
        
        First = tmp;
        Size--;
        return ret;
    }

    
    // remove all elements from list
    void clear()
    {
        if (Size == 0) {
            return;
        }
        
        TItem *tmp = First;
        while (tmp != NULL) {
            TItem* Next = tmp->Next;
            delete tmp;
            tmp = Next;
        }
        
        First = NULL;
        Last = NULL;
        Size = 0;
    }
};



////////////////////////////////////////////////////////////
// baseclass for all threaded classes
// - defines basic interface
////////////////////////////////////////////////////////////

class TThread
{

protected:

    pthread_t ThreadID;
    pthread_attr_t ThreadAttr;
    bool Running;

    void Exit();                                                         // terminate thread
    
public:

    TThread ();

    virtual ~TThread();

    inline void ResetRunning() { Running = false; }                      // resets running status
    virtual void Run() = 0;                                              // actual method the thread executes
    void Create(const bool detached = false, const bool sscope = false); // create thread (actually start it)
    void Detach();                                                       // detach thread
    void Join();                                                         // synchronise with thread (wait until finished)
    void Cancel();                                                       // request cancellation of thread

};



////////////////////////////////////////////////////////////
// Mutex 
////////////////////////////////////////////////////////////

class TMutex
{

protected:

    pthread_mutex_t Mutex;
    pthread_mutexattr_t MutexAttr;

public:

    TMutex ()
    {
        pthread_mutexattr_init(&MutexAttr);
        pthread_mutex_init(&Mutex, &MutexAttr);
    }
    
    ~TMutex ()
    {
        pthread_mutex_destroy(&Mutex);
        pthread_mutexattr_destroy(&MutexAttr);
    }

    // lock and unlock mutex (return 0 on success)
    inline int Lock() { return pthread_mutex_lock(&Mutex); }
    inline int Unlock() { return pthread_mutex_unlock(&Mutex); }

    // return true if mutex is locked, otherwise false
    inline bool IsLocked()
    {
        if (pthread_mutex_trylock(&Mutex) != 0) {
            return true;
        } else {
            Unlock();
            return false;
        }
    }
};


////////////////////////////////////////////////////////////
// Condition
////////////////////////////////////////////////////////////

class TCondition : public TMutex
{

protected:

    pthread_cond_t Condition;

public:

    TCondition() { pthread_cond_init(&Condition, NULL); }
    ~TCondition() { pthread_cond_destroy(&Condition); }

    inline void Wait() { pthread_cond_wait(&Condition, &Mutex ); }
    inline void Signal() { pthread_cond_signal(&Condition); }
    inline void Broadcast() { pthread_cond_broadcast(&Condition); }
};



class TPoolThread;


////////////////////////////////////////////////////////////
// ThreadPool - Maintains a set of threads to run jobs
////////////////////////////////////////////////////////////

class TThreadPool
{
    friend class TPoolThread;
    
public:
    
    class TJob
    {
    private:
        
        TMutex SyncMutex;
        
    public:
        
        TJob() { }
        
        virtual ~TJob()
        {
            if (SyncMutex.IsLocked()) {
                std::cerr << "(TJob) destructor : job is still running!" << std::endl;
            }
        }
        
        virtual void Run(void* ptr) = 0;
        
        void Lock() { SyncMutex.Lock(); }
        void Unlock() { SyncMutex.Unlock(); }
    };
    
protected:

    unsigned int MaxParallel;                      // maximum number of theads (degree of parallelism)
    TPoolThread** Threads;                 // array of threads, managed by pool
    TLinkedList<TPoolThread*> IdleThreads; // idle threads
    TCondition IdleCondition;              // condition for synchronisation of idle list

    TPoolThread* GetIdleThread();
    void AppendIdleThread(TPoolThread* t);
    
public:

    TThreadPool(const unsigned int maxParallel);

    ~TThreadPool();
    
    void Run(TJob* job, void* ptr = NULL, const bool del = false);
    void Sync(TJob* job);
    void SyncAll();

};

#endif  // _TTHREAD_POOL_H_INCLUDED_
#endif  // _WIN32
