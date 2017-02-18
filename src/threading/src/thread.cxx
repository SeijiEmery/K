
#include "../include/thread.hxx"
#include <thread>
#include <atomic>
#include "concurrentqueue.h"

using namespace std;

class ThreadWorkerImpl {
    using moodycamel::ConcurrentQueue;
public:
    friend class KThread;

    ThreadWorkerImpl (KThread& thread, IThreadWorker* worker) : worker(worker) {}
    ~ThreadWorkerImpl () {}

    void run ();
protected:
    std::unique_ptr<IThreadWorker> worker;
    ConcurrentQueue<ThreadTask> tasks;
    atomic<bool> running = false;
};

//
// KThread methods
//

KThread::KThread (IThreadWorker* worker)
    : impl(new ThreadWorkerImpl(*this, worker)) {}

// Push task to be run on thread
void KThread::postTask (ThreadTask& task) {
    impl->emplace(std::move(task));
}

// Get / set thread run state
bool KThread::isRunning () {
    return impl->running;
}
void KThread::setRunning (bool value) {
    impl->running = value;
}

// Helper functions that wrap worker + thread impl calls in exception wrappers
// that call IThreadWorker::onInternalException.
//
// Used to implement KThread::runMainLoop();

static bool initThread (KThread& thread) {
    try {
        impl->worker->onThreadInit(thread);
        return true;
    } catch (const std::exception& e) {
        impl->worker->onInternalException(thread, ThreadErrorLocation::USER_ON_THREAD_INIT, e);
        return false;
    }
}
static void exitThread (KThread& thread) {
    try {
        impl->worker->onThreadExit(thread);
    } catch (const std::exception& e) {
        impl->worker->onInternalException(thread, ThreadErrorLocation::USER_ON_THREAD_EXIT, e);
    }
}
static void runThreadMainLoop (KThread& thread) {
    try {
        impl->run(thread);
    } catch (const std::exception& e) {
        impl->worker->onInternalException(thread, ThreadErrorLocation::INTERNAL_MAIN_LOOP, e);
    }
}


// Runs a thread process. Calls methods in the threadWorker as outlined by the docs in
// thread.hxx, and runs until Thread::isRunning() returns false.
void KThread::runMainLoop () {
    if (impl->running)
        throw new std::runtime_exception("Usage error: thread already running.");
    impl->running = true;

    // Try thread init, and if that fails, kill the thread.
    // And yes, ignore attempts to save it; IThreadWorker::onThreadInit() MUST succeed
    // or we're in a potentially invalid state.
    if (!initThread(*this)) {
        impl->running = false;
        exitThread(*this);
        impl->running = false;
        return;
    }

    // So long as we're still scheduled to run, run the main loop.
    // When / if this fails, we _may_ start up again (eg. internal exception was thrown +
    // the thread main loop bailed, but we "reset" w/ thread.setRunning(true) inside of the
    // exception handler to prevent the thread from actually dying).
    while (impl->running)
        runThreadMainLoop(*this);

    // Signal that our thread process just died.
    exitThread(*this);
}

// Actual implementation for the thread main loop. Called via
//  KThread::runMainLoop() -> runThreadMainLoop() -> KThreadImpl::run()
void KThreadImpl::run (KThread& thread) {
    ThreadTask task;

    // Run tasks until we're signaled to stop.
    while (running) {
        if (!tasks.try_dequeue(task)) {
            // Dequeue failed -- call onThreadTaskAwait
            try {
                worker->onAwaitTasks(thread); 
            } catch (const std::exception& e) {
                if (!worker->onInternalException(thread, ThreadErrorLocation::USER_ON_AWAIT_TASKS), e)
                    throw e;
            }
            continue;
        } else {
            // Dequeue succeeded -- call runTask / onTaskException
            try {
                worker->runTask(thread, task);
            } catch (const std::exception& e) {
                if (!worker->onTaskException(thread, task))
                    throw e;
            }
        }
    }
}
