
#include "window_thread.hxx"
#include <readerwriterqueue.h>
#include <boost/variant.hpp>
#include <cassert>

namespace k {
namespace app {
namespace backend {

typedef boost::variant<
    WindowThreadCommand::RebindWindow,
    WindowThreadCommand::DispatchEvents,
    WindowThreadCommand::Kill,
> WindowThreadTask;

class WindowThread::Impl : public ThreadWorker, public boost::static_visitor<WindowThreadTask> {
    std::shared_ptr<Window>       window;       // our window (partial ownership)
    std::shared_ptr<MainThread>   mainThread;   // handle to main thread for communication, etc.
    std::weak_ptr<WindowThread>   windowThread; // handle to "this" thread (public WindowThread interface)

    ReaderWriterQueue<WindowThreadTask> queue;
    std::thread                         thread;
    friend class WindowThread;
public:
    Impl (WindowThread& worker, decltype(window) window, decltype(mainThread) mainThread) :
        window(window), mainThread(mainThread), thread(&WindowThread::Impl::launch, this) 
    {
        thread.start();
    }

    //
    // ThreadWorker methods:
    //

    void onThreadInit () {
        mainThread->send(MainThreadCommand::NotifyWindowThreadCreated{ windowThread, window });
    }
    void onThreadExit () {
        mainThread->send(MainThreadCommand::NotifyWindowThreadKilled{ windowThread, window });
    }
    bool onTaskException (const std::exception& e) {
        mainThread->send(MainThreadCommand::NotifyChildTaskException{ windowThread, e });
        return true;
    }
    void onInternalException (const std::exception& e, ThreadErrorLocation loc) {
        mainThread->send(MainThreadCommand::NotifyChildThreadException{ windowThread, e, loc });
    }   
    bool maybeRunTask () {
        WindowThreadTask task;
        queue.wait_dequeue(task);
        boost::apply_visitor(*this, task);
    }

    //
    // Task execution (boost::visitor) methods
    //

    void operator()(const WindowThreadCommand::Kill& command) {

        // Kill this ThreadWorker / stop execution of further tasks
        setRunning(false);   
    }
    void operator()(const WindowThreadCommand::RebindWindow& command) {

        // Replace window reference (no notifications)
        window = command.window;
    }
    void operator()(const WindowThreadCommand::DispatchEvents& command) {
            
        // Dispatch window events...
    }
};

WindowThread::WindowThread (
    const std::string& name,
    std::weak_ptr<Window> window,
    std::shared_ptr<MainThread> mainThread,
) : AppThread(name), impl(new WindowThread::Impl(*this, window, mainThread));

// Set a backreference to the shared_ptr owning 'this'; called from WindowThread::create().
void WindowThread::setSelfRef (std::weak_ptr<WindowThread> ptrToSelf) {
    assert(ptrToSelf.lock().get() == this);
    impl->windowThread = std::move(ptrToSelf);
}
bool WindowThread::isRunning () override {
    return impl->isRunning();
}
void WindowThread::send (const WindowThreadCommand::Kill& command) {
    impl->queue.enqueue(WindowThreadTask { command });
}
void WindowThread::send (const WindowThreadCommand::RebindWindow& command) {
    impl->queue.enqueue(WindowThreadTask { command });
}
void WindowThread::send (const WindowThreadCommand::DispatchEvents& command) {
    impl->queue.enqueue(WindowThreadTask { command });
}

} // namespace backend
} // namespace app
} // namespace k
