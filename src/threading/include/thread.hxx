
#pragma once

#include <functional>

namespace k {
namespace thread {

typedef std::function<void()> ThreadTask;

// Used to signal error location in onInternalException.
enum class ThreadErrorLocation {

    // Thrown by user code:
    USER_ON_THREAD_INIT,
    USER_ON_THREAD_EXIT,
    USER_ON_AWAIT_TASKS,

    // Thrown by internal (KThread) code:
    INTERNAL_MAIN_LOOP,
    INTERNAL_UNKNOWN,
};


// Defines thread behavior outside of run + execute tasks.
class IThreadWorker {
    // Called when thread main loop begins (before all tasks).
    virtual void onThreadInit (KThread&) {}

    // Called when thread main loop exits.
    virtual void onThreadExit (KThread&) {}

    // Called to execute each task. User code can filter tasks, etc.
    // Note: exceptions get caught automatically + passed to onTaskException.
    virtual void runTask (KThread&, ThreadTask& task) { task(); }

    // Called when task queue is empty. Can block / sleep, etc.
    virtual void onAwaitTasks (KThread&) {}

    // Called when an exception is thrown while executing a ThreadTask.
    // If returns false, will rethrow exception, which will terminate the run loop.
    virtual bool onTaskException (ThreadTask&, const std::exception&) = 0;

    // Called when an exception is thrown _outside_ of a task context.
    // Not supposed to happen, but must be recovered / logged appropriately.
    //
    // Note: can be called from a variety of contexts, ie thrown from:
    //  1) onThreadInit() / onThreadExit() / onAwaitTasks() / onTaskException()
    //  2) internal code used to implement KThread (not the user's fault)
    //
    // This is specified by the ThreadErrorLocation.
    //
    virtual bool onInternalException (KThread&, ThreadErrorLocation, const std::exception&) = 0;
};

class KThreadImpl;

// Basic, opaque thread class.
// Internally uses moodycamel::ConcurrentQueue (concurrentqueue.h) to store tasks.
class KThread {
    std::unique_ptr<KThreadImpl> impl;
public:
    KThread     (IThreadWorker* worker);
    ~KThread    ();

    // Get / set thread run status.
    bool isRunning ();
    void setRunning (bool running);

    // Post a thread task to run on this thread.
    void postTask (const ThreadTask& task);

    // Launch the main thread. If already running throws a std::runtime_exception.
    void runMainLoop ();
};

}; // namespace thread
}; // namespace k
