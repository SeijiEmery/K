
#pragma once

#include "main_thread.hxx"
#include "base_app_thread.hxx"
#include "util/command_buffer.hxx"      // maybe use this, or replace w/ boost::variant

namespace k {
namespace app {
namespace backend {

// Commands that can be sent to / run on a WindowThread:
namespace WindowThreadCommand {

    // Kill target thread
    struct Kill { std::weak_ptr<AppThread> killedFrom };
    
    // Change target window
    struct RebindWindow {
        std::weak_ptr<Window> window;     // can be null
    };

    // Dispatch an event list (dispatches all relevant events via AppWindow methods on the target thread)
    struct DispatchEvents {
        std::unique_ptr<CommandBuffer<kAppEvent>> events;
    };
} // namespace WindowThreadCommand


// Encapsulates a std::thread, message queue, and ThreadWorker that partially owns an AppWindow object
// (shared ownership + access with the main thread).
//
// All thread operations are done via the message queue - eg. to kill this thread, call
//   <thread>.send(WindowThreadCommand::Kill{ <your thread> });
//
// Inherits from AppThread, which provides thread tracking / registration + a public name() method.
//
class WindowThread : public AppThread {
    class Impl;
    std::unique_ptr<Impl> impl;
private:
    // Creates + runs a thread w/ partial ownership of window and responsibility
    // for running per-thread tasks on that window (GL calls, etc).
    WindowThread (
        std::string                  name,
        std::shared_ptr<AppWindow>&  window,
        std::shared_ptr<MainThread>& mainThread,
    );
    void setSelfRef (std::weak_ptr<WindowThread>);
public:
    // AppThread methods
    bool isRunning () override;

    // Send a message (Command) to be run on that window.
    // WARNING:
    //  Uses a single producer / single consumer queue internally (moodycamel::ReaderWriterQueue),
    //  so these methods should ONLY be called from the main thread (which owns + manages multiple
    //  child AppThreads).
    //
    //  Likewise, AppWindow is only expected to be accessed from this thread (outside of event
    //  collection, which must be run on the main thread).
    //

    void send (const WindowThreadCommand::Kill&);
    void send (const WindowThreadCommand::RebindWindow&);
    void send (const WindowThreadCommand::DispatchEvents&);

    template <typename... Args>
    static auto create (Args... args) {
        auto ptr = std::make_shared<WindowThread>(args...);
        ptr->setSelfRef({ ptr });
        return ptr;
    }
};


} // namespace backend
} // namespace app
} // namespace k
