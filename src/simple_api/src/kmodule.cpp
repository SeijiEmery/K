
#include "kmodule.hpp"
#include <vector>

struct ModuleStatusFlags {
    NEEDS_INIT         = 1 << 1,
    NEEDS_FLAG_UPDATE  = 1 << 2,
    NEEES_RELOAD       = 1 << 3,
    NEEDS_TEARDOWN     = 1 << 4,
    MODULE_ACTIVE      = 1 << 5,
};

template <unsigned T, unsigned F>
static bool setFlags (ModuleFlags& flags, ModuleFlags& value) {
    if ((value & T != 0) != (value & F != 0)) {
        flags &= ~(T | F);
        flags |= value & (T | F);
        return true;
    }
    return false;
}
template <unsigned T, unsigned F>
static ModuleFlags getFlag (ModuleFlags flags) {
    return flags & (T | F);
}

template <unsigned T, unsigned F>
static ModuleFlags getFlagDiff (ModuleFlags a, ModuleFlags b) {
    auto v = (ModuleFlags)(a & (T | F));
    return v == (b & (T | F)) ? v : ModuleFlags::NONE;
}

class ModuleReference::Impl {};
struct KModule : public ModuleReference {
    ModuleManager*           moduleManager;
    std::unique_ptr<IModule> module;
    std::string              path;
    ModuleState              state;
    std::atomic<ModuleFlags> flags;
    ModuleFlags              prevFlags;
    std::atomic<ModuleStatusFlags> statusFlags;

    std::vector<EventListener> eventListeners;
    std::vector<std::tuple<SubProcess, KModule*, std::chrono::duration, bool>> callTimeInfo;

    KModule (ModuleManager& mgr, IModule* module, const std::string& path, ModuleFlags flags) :
        ModuleReference(nullptr),
        moduleManager(&mgr),
        module(module),
        path(path),
        flags(flags), 
        prevFlags(flags),
        statusFlags(ModuleStatusFlags::NEEDS_INIT)
    {
        assert(module != nullptr);
    }
    template <typename F>
    void run (SubProcess subprocess, KModule* owner, const F& function) {
        SomeStopwatchType sw; sw.begin();
        try {
            function();
            callTimeInfo.push_back(subprocess, owner, sw.elapsed(), true);
        } catch (const std::exception& e) {
            callTimeInfo.push_back(subprocess, owner, sw.elapsed(), false);
            moduleManager->notifyException(owner, subprocess, e);
        } catch {
            callTimeInfo.push_back(subprocess, owner, sw.elapsed(), false);
            moduleManager->notifyException(owner, subprocess, std::runtime_error("Unknown exception"));
        }
    }

    template <unsigned eventId, typename... Args>
    void dispatchEvent (Args... args) {
        for (size_t i = eventListeners.size(); i --> 0; ) {
            if (eventListeners[i].module.expired()) {
                if (i != eventListeners.size()-1)
                    std::swap(eventListeners[i], eventListeners[eventListeners.size()-1]);
                eventListeners.pop_back();
            } else if (eventListeners[i].type == eventId) {
                run<Subprocess::STATE_CHANGE_EVENT>(
                    eventListeners[i].module,
                    [&](){ eventListeners[i].callback(args...); });
            }
        }
    }

    template <unsigned T, unsigned F>
    void updateFlags () {
        while (auto v = flags & (T | F); v != prevFlags & (T | F)) {
            assert((v & T != 0) != (v & F != 0));
            prevFlags &= ~(T | F);
            prevFlags |= v;
            dispatchEvent<EventType::ON_FLAG_CHANGED>(*this, v);
        }
    }
    void processFrame (FrameState& frame) {
        callTimeInfo.clear();

        // Update state...

        if (statusFlags & ModuleStatusFlags::NEEDS_INIT) {
            statusFlags &= ~ModuleStatusFlags::NEEDS_INIT;
            doInit(frame);
        }
        if (statusFlags & ModuleStatusFlags::NEEDS_RELOAD) {
            statusFlags &= ~ModuleStatusFlags::NEEDS_RELOAD;
            doTeardown(frame);
            doInit(frame);
        }
        if (statusFlags & ModuleStatusFlags::NEEDS_TEARDOWN) {
            statusFlags &= ~ModuleStatusFlags::NEEDS_TEARDOWN;
            doTeardown(frame);
        }
        if (statusFlags & ModuleStatusFlags::NEEDS_FLAG_UPDATE) {
            updateFlags<ModuleFlags::RUN_ON_FRAME, ModuleFlags::PAUSE_ON_FRAME>();
            updateFlags<ModuleFlags::RUN_ON_GL,    ModuleFlags::PAUSE_ON_FRAME>();
        }
        if (flags & ModuleFlags::MODULE_ACTIVE && flags & ModuleFlags::RUN_ON_FRAME) {
            assert(module);
            run<Subprocess::ON_FRAME>(*this, [&](){ module->onFrame(state, frame); });
        }

        // Window updates, etc., TBD.
    }
    void processGL (GLContext& context) {
        if (flags & ModuleFlags::MODULE_ACTIVE && flags & ModuleFlags::RUN_ON_GL) {
            assert(module);
            run<Subprocess::ON_GL>(*this, [&](){ module->onGL(context): });
        }
    }
    void setFlags (ModuleFlags value) {
        if (setFlags<ModuleFlags::RUN_ON_FRAME, ModuleFlags::PAUSE_ON_FRAME>(flags, value) |
            setFlags<ModuleFlags::RUN_ON_GL,    ModuleFlags::PAUSE_ON_GL>   (flags, value))
        {
            statusFlags |= ModuleStatusFlags::NEEDS_FLAG_UPDATE;
        }
    }
    void doInit (FrameState& frame) {
        if (!(flags & ModuleFlags::MODULE_ACTIVE)) {
            flags |= ModuleFlags::MODULE_ACTIVE;
            if (run<SubProcess::INIT>(*this, [&](){ module->init(state, frame); })) {
                dispatchEvent<EventType::ON_LOADED>(*this);
            }
        }
    }
    void doTeardown (FrameState& frame) {
        if (flags & ModuleFlags::MODULE_ACTIVE) {
            flags &= ~ModuleFlags::MODULE_ACTIVE;
            dispatchEvent<EventType::ON_CLOSED>(*this);
            run<SubProcess::TEARDOWN>(*this, [&](){ module->init(state, frame); });
        }
    }
};

const std::string&  ModuleReference::name () const { 
    return static_cast<KModule*>(this)->state.name; 
}
const std::string&  ModuleReference::path () const { 
    return static_cast<KModule*>(this)->path; 
}
ModuleFlags ModuleReference::getFlags () const {
    return *static_cast<KModule*>(this)->flags;
}
void ModuleReference::setFlags (ModuleFlags value) {
    static_cast<KModule*>(this)->setFlags(value);   
}
IModule* ModuleReference::getPtr () {
    return static_cast<KModule*>(this)->module.ptr();
}
bool ModuleReference::reload () {
    static_cast<KModule*>(this)->statusFlags |= ModuleStatusFlags::NEEDS_RELOAD;
}
bool ModuleReference::close () {
    static_cast<KModule*>(this)->statusFlags |= ModuleStatusFlags::NEEDS_TEARDOWN;
}
void ModuleReference::dispatchOnLocalThread (std::function<void()> callback) {
    // TODO: implement this properly!
    callback();
}
void ModuleReference::onLoaded (
    ModuleRef& caller, 
    std::function<void(ModuleReference&)> callback
) {
    static_cast<KModule*>(this)->addEventListener(caller, EventType::ON_LOADED, callback);
}
void ModuleReference::onClosed (
    ModuleRef& caller, 
    std::function<void(ModuleReference&)> callback
) {
    static_cast<KModule*>(this)->addEventListener(caller, EventType::ON_CLOSED, callback);
}
void ModuleReference::onFlagChanged (
    ModuleRef& caller, 
    std::function<void(ModuleReference&, ModuleFlags changedFlag)> callback
) {
    static_cast<KModule*>(this)->addEventListener(caller, EventType::ON_FLAG_CHANGED, callback);
}

class ModuleManager::Impl {
    std::vector<ModuleRef>  modules;
    std::mutex              mutex;
    FrameState              frameState[2];
    unsigned                currentFrame = 0;
    static ModuleRef        g_nullModule;

    std::vector<std::tuple<SubProcess, KModule*, std::chrono::duration, bool>> callTimeInfo;

    ModuleRef& loadModule (IModule* module, ModuleFlags flags) {
        std::lock lock (mutex);
        auto ptr = std::static_ptr_cast<ModuleReference>(std::make_shared<KModule>(*this, module, "", flags));
        modules.push_back(std::move(ptr));
        return modules.back();
    }

    void updateFrame () override {
        // update frame state...
        ++currentFrame;
        callTimeInfo.clear();
        for (auto& module : modules) {
            module->processFrame(frameState[currentFrame&1]);
            std::copy(callTimeInfo.end(), module->callTimeInfo.begin(), module->callTimeInfo.end());
        }
    }
    void updateGL (GLContext& context) override {
        for (auto& module : modules)
            module->processGL(context);
    }
    ModuleRef& getModule (const std::string& name) {
        for (auto& module : modules)
            if (module->name() == name)
                return module;
        return g_nullModule;
    }
};

ModuleRef& loadModule (
    const std::string& path, 
    bool reload = false, 
    ModuleFlags flags = ModuleFlags::DEFAULT
) {
    static_assert(0, "unimplemented");
}
ModuleRef& loadModule (
    IModule* module,
    ModuleFlags flags = ModuleFlags::DEFAULT
) {
    return impl->loadModule(module, flags);
}
ModuleRef& getModule (const std::string& name) {
    return impl->getModule(name);
}
const std::vector<ModuleRef>& modules () const {
    return impl->modules;
}

