
#pragma once
#include "kwindow.hpp"
#include "kinput.hpp"
#include "kgamepad.hpp"
#include "kthreads.hpp"

// Flags to control module run state.
//
// Note: most of these are implemented as a T,F pair (ie. RUN_X | PAUSE_X).
// Only T or F will ever be set at a given time. For setFlags() we work as follows:
//   T => set T, F => set F, (T & F) or !(T | F) => no change.
//
enum class ModuleFlags {
    // Control whether the onFrame() method is called (or paused).
    RUN_ON_FRAME = 1 << 1, PAUSE_ON_FRAME = 1 << 2,

    // Control whether the onGL() method is called (or paused)
    RUN_ON_GL = 1 << 3, PAUSE_ON_GL = 1 << 4,
};


// Data structure used to reference + control an IModule.
struct ModuleReference {
    friend class ModuleManager;

    // Get name of module (set indirectly using ModuleState::name).
    const std::string&  name () const;

    // Get path to loaded module (a .kmodule file).
    const std::string&  path () const;

    // Get module flags (run state)
    ModuleFlags         getFlags () const;

    // Set module flags. 
    // Since flags are T / F pairs, passing 0 for a given flag will NOT clear it.
    // As such, the following are equivalent:
    //      module->setFlags(ModuleFlags::PAUSE_ON_FRAME);
    //      module->setFlags(module->getFlags() | ModuleFlags::PAUSE_ON_FRAME);
    //
    void                setFlags (ModuleFlags flags);

    // Get a direct refence to a loaded module. Useful, since modules can declare their own
    // interfaces in header files and thus communicate abstractly with other modules.
    //
    // If you use this method, you MUST be aware of the following:
    // – IModules can be loaded / unloaded at any time, so do NOT store this pointer anywhere.
    // – The returned pointer is guaranteed to be valid for the duration of an init(), teardown(),
    //   onFrame(), or onGL() method, or in one of the following callbacks, but that's it.
    // – It would be helpful to think of ModuleReference akin to a std::weak_ptr, since that's
    //   essentially what it is (plus a bunch of methods for controlling + observing module state).
    // – Modules MAY be running on their own threads. For safety, wrap any calls in getThread()->dispatch().
    //
    // Returns nullptr if the module is not currently loaded.
    //
    IModule* getPtr ();

    // Returns true if getPtr() != nullptr (ie. there exists a valid, loaded IModule object to dereference).
    operator bool () const { return getPtr() != nullptr; }

    // Typesafe, safer equivalent to getPtr(), provided you know *exactly* what the type concrete
    // type of your module is (which you'll likely be getting via a string...). 
    // Returns a reference, so throws an std::runtime_error if getPtr() returned null.
    // Use operator bool() to check 
    template <typename T>
    T& getAs () { 
        if (!*this) throw std::runtime_error("Null module reference");
        return *dynamic_cast<T*>(getPtr());
    }

    // Force module to load / reload; returns true iff successful.
    bool reload ();

    // Force module to close; returns true iff module was running (and is now closed).
    bool close  ();

    std::weak_ptr<KThread> getThread ();

    // Event listener methods. These have the following signature: (caller, callback).
    // A reference to the caller (an IModule) is required for lifetime management.

    // Called after IModule::init(). Closure is called on module's thread.
    void onLoaded (IModule& caller, std::function<void(ModuleReference&)>);

    // Called before IModule::teardown(). Closure is called on target module's thread.
    void onClosed (IModule& caller, std::function<void(ModuleReference&)>);

    // Called on each flag (run state) change. ChangedFlag consists of a single flag only.
    void onFlagChanged (IModule& caller, std::function<void(ModuleReference&, ModuleFlags changedFlag)>);
private:
    class Impl;
    ModuleReference (Impl* impl) : impl(impl) {}
    ModuleReference (const ModuleReference&) = delete;
    ModuleReference& operator= (const ModuleReference&) = delete;
    ~ModuleReference () = default;
    std::unique_ptr<Impl> impl;
};
typedef std::shared_ptr<ModuleReference> ModuleRef;


// 
struct ModuleManager {
    void registerInterface (IModule& module, const std::string& name, void* interface, ThreadMask targetThread = ThreadMask::ANY);
    bool acquireInterface  (const std::string& name, std::function<void(void*)> callback);

    // Try to load module at the given path, and/or reload module at that path (iff reload true).
    // Returns a null reference if module not loaded / reloaded.
    ModuleRef& loadModule (const std::string& path, bool reload = false, ModuleFlags flags = ModuleFlags::DEFAULT);

    // Get list of all active modules.
    const std::vector<ModuleRef>& modules () const;


    ModuleRef& getModule (const std::string& name);
private:
    class Impl;
    ModuleManager (Impl* impl) : impl(impl) {}
    ModuleManager (const ModuleManager&) = delete;
    ModuleManager& operator= (const ModuleManager&) = delete;
    ~ModuleManager () = default;
    std::unique_ptr<Impl> impl;
};






struct ModuleState {
    // State for this module
    std::string name;   // our public name (settable)
    ThreadMask  onFrameThread = ThreadMask::ANY;    // required thread to run onFrame() on.
    ModuleRef   module; // this module; implements controls, etc

    // Module intercommunication TBD

    // Frame state (current frame). Non-null; readonly.
    const Time*         time;
    const Mouse*        mouse;
    const Keyboard*     keyboard;
    const GamepadList*  gamepads;

    // Interfaces for window, thread, and app management
    WindowList          windows;
    const ScreenList*   screens;
    ThreadManager*      threads;
    ModuleManager*      modules;
};
struct GLContext {
    // TBD...
};

class IModule {
    friend class ModuleManager;
protected:
    virtual void init     (ModuleState& state) = 0;
    virtual void frame    (ModuleState& state) = 0;
    virtual void onGL     (GLContext& context) = 0;     
    virtual void teardown (ModuleState& state) = 0;
};
