// Thread Pool - multithreading, used for training the trees 
// Original code by Cliff Brunk, modified by Daria Sorokina. Yandex Labs, 2010.

#ifndef _WIN32

#include <unistd.h>
#include <sched.h>
#include <pthread.h>


#include <signal.h>
#include <time.h>
#include <string.h>

#include <iostream>
#include <cmath>

#include "thread_pool.h"

// set to one to enable sequential execution
#define THR_SEQUENTIAL  0


extern "C" void * RunThread(void *arg)
{
    if (arg != NULL) {
        ((TThread*) arg)->Run();
        ((TThread*) arg)->ResetRunning();
    }
    
    return NULL;
}


TThread::TThread()
    : Running(false)
{ }


TThread::~TThread()
{
    // request cancellation of the thread if running
    if (Running) {
        Cancel();
    }
}

  
// create thread (actually start it)
void TThread::Create(const bool detached, const bool sscope)
{
    if (!Running) {
        int status;
        
        if ((status = pthread_attr_init(&ThreadAttr )) != 0) {
            std::cerr << "(TThread) create : pthread_attr_init (" << strerror(status) << ")" << std::endl;
            return;
        }

        if (detached) {
            // detache created thread from calling thread
            if ((status = pthread_attr_setdetachstate(&ThreadAttr, PTHREAD_CREATE_DETACHED)) != 0) {
                std::cerr << "(TThread) create : pthread_attr_setdetachstate (" << strerror(status) << ")" << std::endl;
                return;
            }
        }
        
        if ( sscope ) {
            // use system-wide scheduling for thread
            if ((status = pthread_attr_setscope(&ThreadAttr, PTHREAD_SCOPE_SYSTEM)) != 0 ) {
                std::cerr << "(TThread) create : pthread_attr_setscope (" << strerror(status) << ")" << std::endl;
                return;
            }
        }
        
        if ((status = pthread_create(&ThreadID, &ThreadAttr, RunThread, this) != 0)) {
            std::cerr << "(TThread) create : pthread_create (" << strerror(status) << ")" << std::endl;
        } else {
            Running = true;
        }
    }
    else
        std::cout << "(TThread) create : thread is already running" << std::endl;
}


void TThread::Detach()
{
    if (Running) {
        int status;
        if ((status = pthread_detach(ThreadID)) != 0) {
            std::cerr << "(TThread) detach : pthread_detach (" << strerror(status) << ")" << std::endl;
        }
    }
}


// synchronise with thread (wait until finished)
void TThread::Join()
{
    if (Running) {
        int status;
        if ((status = pthread_join(ThreadID, NULL)) != 0) {
            std::cerr << "(TThread) join : pthread_join (" << strerror(status) << ")" << std::endl;
        }
        Running = false;
    }
}


// request cancellation of thread
void TThread::Cancel()
{
    if (Running) {
        int status;
        if ((status = pthread_cancel(ThreadID)) != 0) {
            std::cerr << "(TThread) cancel : pthread_cancel (" << strerror(status) << ")" << std::endl;
        }
    }
}


// terminate thread
void TThread::Exit()
{
    if (Running && (pthread_self() == ThreadID)) {
        void* returnValue = NULL;
        pthread_exit(returnValue);
        Running = false;
    }
}


/*
// put thread to sleep (milli + nano seconds)
void TThread::Sleep(const double sec)
{
    if (Running) {
        struct timespec interval;

        if (sec <= 0.0) {
            interval.tv_sec = 0;
            interval.tv_nsec = 0;
        } else {
            interval.tv_sec = time_t(std::floor(sec));
            interval.tv_nsec = long((sec - interval.tv_sec) * 1e6);
        }
        
        nanosleep(&interval, 0);
    }
}
*/


//////////////////////////////////////////////////////////////////////////
// thread handled by threadpool
//////////////////////////////////////////////////////////////////////////

class TPoolThread : public TThread
{
protected:

    TThreadPool* Pool;          // thread pool to which this thread belongs
    
    TThreadPool::TJob* Job;     // job
    void* JobDataPtr;           // job data
    
    bool DeleteJob;             // should the job be deleted upon completion
    TCondition WorkCondition;   // condition for job-waiting
    bool End;                   // indicates end-of-thread
    TMutex DeleteMutex;         // mutex for preventing premature deletion
    
public:

    TPoolThread (TThreadPool* pool)
        : TThread(),
          Pool(pool),
          Job(NULL),
          JobDataPtr(NULL),
          DeleteJob(false),
          End(false)
    { }
    
    ~TPoolThread() {}
    
    
    // parallel running method
    void Run()
    {
        DeleteMutex.Lock();
        
        while (!End) {
            
            // append thread to idle-list and wait for work
            Pool->AppendIdleThread(this);
            
            WorkCondition.Lock();
            
            while ((Job == NULL ) && !End ) {
                WorkCondition.Wait();
            }

            WorkCondition.Unlock();
            
            // determine if there is job to run and run it
            if (Job != NULL) {
                
                // execute job
                Job->Lock();
                Job->Run(JobDataPtr);
                Job->Unlock();
                
                if (DeleteJob) {
                    delete Job;
                }

                // reset data
                WorkCondition.Lock();
                Job = NULL;
                JobDataPtr = NULL;
                WorkCondition.Unlock();
            }
        }
        
        DeleteMutex.Unlock();
    }
    

    // set and run job with optional data
    void RunJob(TThreadPool::TJob* job, void* jobDataPtr, const bool deleteJob = false)
    {
        WorkCondition.Lock();
        
        Job = job;
        JobDataPtr = jobDataPtr;
        DeleteJob = deleteJob;
        
        WorkCondition.Signal();  // restart one of the threads waiting on the WorkCondition 
        WorkCondition.Unlock();
    }


    // give access to delete mutex
    TMutex& GetDeleteMutex()
    {
        return DeleteMutex;
    }


    // quit thread (reset data and wake up)
    void Quit()
    {
        WorkCondition.Lock();
        
        End = true;
        Job = NULL;
        JobDataPtr = NULL;
        
        WorkCondition.Signal();
        WorkCondition.Unlock();
    }
    
};

    

//////////////////////////////////////////////////////////////////////////
// ThreadPool - implementation
//////////////////////////////////////////////////////////////////////////


// constructor and destructor
TThreadPool::TThreadPool(const unsigned int maxParallel)
{

    MaxParallel = maxParallel;
    
    Threads = new TPoolThread*[MaxParallel];

    if (Threads == NULL) {
        MaxParallel = 0;
        std::cerr << "(TThreadPool) TThreadPool : could not allocate thread array" << std::endl;
    }
    
    for (unsigned int i = 0; i < MaxParallel; i++ ) {
        Threads[i] = new TPoolThread(this);
        
        if (Threads == NULL) {
            std::cerr << "(TThreadPool) TThreadPool : could not allocate thread" << std::endl;
        } else {
            Threads[i]->Create(true, true);
        }
    }
}


TThreadPool::~TThreadPool()
{
    // wait till all threads have finished
    SyncAll();
    
    // finish all thread
    for (unsigned int i = 0; i < MaxParallel; i++) {
        Threads[i]->Quit();
    }

    // cancel and delete all threads (not really safe !)
    for (unsigned int i = 0; i < MaxParallel; i++) {
        Threads[i]->GetDeleteMutex().Lock();
        delete Threads[i];
    }
    
    delete[] Threads;
}


void TThreadPool::Run(TThreadPool::TJob* job, void* jobDataPtr, const bool deleteJob)
{
    if (job == NULL) {
        return;
    }
    
    TPoolThread* thread = GetIdleThread();
    thread->RunJob(job, jobDataPtr, deleteJob);
}


// wait until <job> was executed
void TThreadPool::Sync(TJob* job)
{
    if (job == NULL) {
        return;
    }
    
    job->Lock();
    job->Unlock();
}


// wait until all jobs have been executed
void TThreadPool::SyncAll()
{
    while (true) {
        IdleCondition.Lock();

        // wait until next thread becomes idle
        if (IdleThreads.size() < MaxParallel) {
            IdleCondition.Wait();
        } else {
            IdleCondition.Unlock();
            break;
        }
        
        IdleCondition.Unlock();
    }
}


// return idle thread form pool
TPoolThread* TThreadPool::GetIdleThread()
{
    while (true) {

        // wait for an idle thread
        IdleCondition.Lock();
        
        while (IdleThreads.size() == 0) {
            IdleCondition.Wait();
        }
        
        // get first idle thread
        if (IdleThreads.size() > 0) {
            TPoolThread* thread = IdleThreads.pop_front_and_return();
            IdleCondition.Unlock();
            return thread;
        }
        
        IdleCondition.Unlock();
    }
}


// append recently finished thread to idle list
void TThreadPool::AppendIdleThread(TPoolThread* thread)
{

    IdleCondition.Lock();
    
    // if thread is already in idle list do not insert it again
    for (TLinkedList<TPoolThread*>::iterator iter = IdleThreads.first(); !iter.eol(); ++iter) {
        if (iter() == thread) {
            IdleCondition.Unlock();
            return;
        }
    }
    
    IdleThreads.push_back(thread);
    
    // wake a blocked thread for job execution
    IdleCondition.Signal();

    IdleCondition.Unlock();
}

#endif
