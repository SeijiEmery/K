
#pragma once

#include "app_config.hxx"
#include "app_frame_info.hxx"
#include "app_window_manager.hxx"
#include "app_thread_manager.hxx"
#include "app_device_manager.hxx"
#include "app_event_manager.hxx"

namespace k {
namespace app {

// Public handle to the main application state.
// Provides buffered, threadsafe access to thread, device, window, and event management.
struct AppInstance {
    FrameInfo     frame;    // time, dt, etc
    ThreadManager thread;   // thread creation + management
    WindowManager window;   // window creation + management
    DeviceManager device;   // input device querying (state, etc)
    EventManager  event;    // event querying (register event listeners, etc)
    //AppLogger     log;      // thread-safe logging subsystem

    // Note: this class is NOT user creatable or copyable.
    AppInstance ()                              = delete;
    AppInstance (const AppInstance&)            = delete;
    AppInstance (AppInstance&&)                 = delete;
    AppInstance& operator= (const AppInstance&) = delete;
    AppInstance& operator= (AppInstance&&)      = delete;

    // And has a hidden, non-public ctor + dtor
    AppInstance (void*);
    ~AppInstance ();

    // You can, however, get a permanent reference to this class though:
    std::shared_ptr<AppInstance> getRef ();
};

// Application behavior gets implemented via 'clients' using this interface.
// This allows us to abstract GLFW window + event handling, the underlying theading
// system, etc., via a high-level interface.
//
// Example -- registering a per-frame callback, and several logging functions.
//
// using namespace k::app;
// class MyExample : public AppClient {
//     void onAppInit (AppInstance& app) override {
//         app.log.printfln("App created! time = %f, dt = %f", app.time.localTime, app.time.dt);
//
//         app.event.onFrame.connect(this, &MyExample::onFrameUpdate);   
//
//         app.event.onAppExitError.connect(this, [](AppInstance& app, const std::exception& exception) {
//              app.log.printfln("App crashed! error: %s", exception.what().c_str());
//         });
//     }
//     void onFrameUpdate (AppInstance& app) {
//         app.log.printfln("Update! time = %f, dt = %f", app.time.localTime, app.time.dt);
//
//         // Do stuff...
//     }
//     void onAppTeardown (AppInstance& app) override {
//         app.log.printfln("App teardown! time = %f, dt = %f", app.time.localTime, app.time.dt);
//     }
// };
//
struct AppClient : public EventAnchor {
    virtual void onAppInit      (AppInstance& app) {}
    virtual void onAppTeardown  (AppInstance& app) {}
};

// Launches an application.
// Usage:
//  int main (size_t argc, const char** argv) {
//      AppLauncher launcher;
//      AppConfig config { ... };
//      launcher.registerClients<
//          MyClient1, 
//          MyClient2, 
//          ...
//       >();
//      return launcher.launch(config, argc, argv);
//  }
//
class AppLauncher {
    void* impl;
public:
    AppLauncher ();
    ~AppLauncher ();

    // Launches the application.
    int  launch   (AppConfig config, size_t argc, const char** argv);

    // Add an application 'client', which can use the public k::app:: interface,
    // instead of dealing directly w/ GLFW, threads, low-level window + event management, etc.
    void addClient (std::unique_ptr<AppClient>);

    // Register one or more clients (helper methods; enables launcher.registerClients<T1,T2,T3,...>()
    // instead of
    //      launcher.addClient(std::unique_ptr<AppClient>(new T1()));
    //      launcher.addClient(std::unique_ptr<AppClient>(new T2()));
    //      launcher.addClient(std::unique_ptr<AppClient>(new T3()));
    //      ...
    //
    template <class Client>
    void registerClient () { addClient(std::unique_ptr<AppClient>(new Client())); }
    
    template <class Client, class... Clients>
    void registerClients () { registerClient<Client>(); registerClients<Clients...>(); }
};

}; // namespace app
}; // namespace k
