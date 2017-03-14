
// NEW

template <typename T>
struct IProperty {
    virtual const T& get () const = 0;
    virtual void set (const T&)   = 0;
};

class WindowModel {
    struct State {
        glm::vec2   pos, size, scale;
        std::string title;
    };
    State state;
    // std::unique_ptr<GLFWwindow> window;
    GLFWwindow* window;
public:
    #define GET_THIS(prop) (reinterpret_cast<WindowModel*>((void*)this – offsetof(WindowModel, prop)))
    #define DEFN_PROPERTY(prop, setter) \
    struct Property##_name : public IProperty<decltype(GET_THIS(prop)->state.prop)> { \
        const auto& get () const { return GET_THIS(prop)->state.prop; } \
        void set (const decltype(get())& v) { GET_THIS(prop)->setter(v); } \
    } prop;

    DEFN_PROPERTY(pos, setPos)
    DEFN_PROPERTY(size, setSize)
    DEFN_PROPERTY(scale, setScale)
    DEFN_PROPERTY(title, setTitle)

    #undef DEFN_PROPERTY
    #undef GET_THIS

    void create () {
        if (!KThread::isMainThread()) {
            KThread::dispatchMainThread([this](){ create(); });
        } else {
            // create GLFWwindow...
        }
    }
    ~WindowModel () {
        if (window) KThread::dispatchMainThread([window](){
            glfwDestroyWindow(window);
        });
    }
    template <typename Archive>
    Archive& serialize (Archive& ar) {
        ar & state.pos & state.size & state.scale & state.title;
    }
protected:
    void setPos (const decltype(state.pos)& v) {
        KThread::dispatchMainThread([this,v](){
            if (pos != v) {

            }
        });
    }
    void setSize (const decltype(state.size)& v) {
        KThread::dispatchMainThread([this,v](){

        });
    }
    void setScale (const decltype(state.scale)& v) {
        KThread::dispatchMainThread([this,v](){

        });
    }
    void setTitle (const decltype(state.title)& v) {
        KThread::dispatchMainThread([this,v](){

        });
    }
    void setVisible (const decltype(state.visible)& v) {

    }

    struct {
        glm::vec2 pos, size, fbSize;
    } eventCollector;

    void eventPreUpdate () {
        eventCollector.pos = state.pos;
        eventCollector.size = state.size;
        eventCollector.fbSize = state.size * state.scale;
    }
    void eventPostUpdate () {
        if (eventCollector.pos != state.pos) {
            // dispatch window pos changed event...
        }
        if (eventCollector.size != state.size) {
            // dispatch window size changed event...
        }
        if (eventCollector.fbSize)
    }

    #define GET_THIS(window) static_cast<WindowModel*>(glfwGetWindowUserPointer(window))
    static void onPositionChanged (GLFWwindow* window, int x, int y) {
        GET_THIS(window)->eventCollector.pos = glm::vec2 { x, y };
    }    
    static void onPositionChanged (GLFWwindow* window, int x, int y) {
        GET_THIS(window)->eventCollector.size = glm::vec2 { x, y };
    }    
    static void onPositionChanged (GLFWwindow* window, int x, int y) {
        GET_THIS(window)->eventCollector.fbsize = glm::vec2 { x, y };
    }





};

// Example (ie. why we have properties setup the way we do)

class WindowViewWidget {
    std::shared_ptr<WindowModel> model;

    void createLayout (UILayoutWidget layout) {
        // Assume model lifetime > widget lifetime. True if this widget object owns widget + model.
        layout.addField(layout.READ | layout.WRITE, "position", &model.position);
        layout.addField(layout.READ | layout.WRITE, "size",     &model.size);
        layout.addField(layout.READ | layout.WRITE, "scale",    &model.scale);
        layout.addField(layout.READ | layout.WRITE, "title",    &model.title);
    }
};

class KThread {
public:
    static std::unique_ptr<KThread> mainThread;
    using moodycamel::ConcurrentQueue;

    static void dispatchMainThread (std::function<void()> cb) {
        mainThread.dispatch(cb);
    }

    void dispatch (std::function<void()> cb) {
        queue.enqueue(cb);
    }
protected:
    ConcurrentQueue<std::function<void()>   queue;
    std::atomic<bool>                       keepRunning;

    void flushQueue () {
        std::function<void()> item;
        while (queue.try_dequeue(item)) {
            try {
                item();
            } catch (const std::exception& e) {
                onTaskException(e);
            } catch {
                onTaskException(std::runtime_error("Unknown error"));
            }
        }
    }
    virtual void onTaskException (const std::exception& e) {
        dispatchMainThread([this, e](){
            std::cerr << "Task exception on thread " << *this << ": " << e.what() << '\n';
        });
    }
    virtual void onFrameException (const std::exception& e) {
        dispatchMainThread([this, e](){
            std::cerr << "task exception on thread " << *this << ": " << e.what() << '\n';
        });
    }
    void run () {
        keepRunning = true;
        while (keepRunning) {
            try {
                flushQueue();
                runFrame();
                flushQueue();
            } catch (const std::exception& e) {
                onFrameException(e);
            } catch {
                onFrameException(std::runtime_error("unknown error"));
            }
        }
    }






};



class AppManager {

};






// OLD

namespace backend {
    class AppWindow {
        glm::vec2                    pos, size, scale;
        std::string                  title;
        std::unique_ptr<GLFWwindow>  window;
        std::weak_ptr<AppWindow>     selfRef;

        AppWindow () {}
    public:
        static std::shared_ptr<AppWindow> create () {
            auto ptr = std::make_shared<AppWindow>();
            ptr->selfRef = std::weak_ptr<AppWindow>(ptr);
            return ptr;
        }

        void setPos (decltype(pos) v) { if (v != pos) cq->pushCommand<SetPos>(selfRef, v); }
        void setSize (decltype(size) v) { if (v != size) cq->pushCommand<SetSize>(selfRef, v); }
        void setScale (decltype(scale) v) { if (v != scale) cq->pushCommand<SetScale>(selfRef, v); }
        void setTitle (decltype(title) v) { if (v != title) cq->pushCommand<SetTitle>(selfRef, v); }

        // Commands
        struct Create {
            std::weak_ptr<AppWindow> window;
            enum { id = AppCommand::kCreateWindow, EXEC_TARGET = MAIN_THREAD_ONLY };
            void execute (AppContext& context) {
                if (auto w = window.lock() && !w->window) {
                    glfwWindowHint(GLFW_RESIZABLE,  w->windowCreationInfo.resizable);
                    glfwWindowHint(GLFW_RESIZABLE,  w->windowCreationInfo.visible);
                    glfwWindowHint(GLFW_RESIZABLE,  w->windowCreationInfo.decorated);

                    glfwWindowHint(GLFW_RED_BITS,   w->windowCreationInfo.colorDepth.r);
                    glfwWindowHint(GLFW_GREEN_BITS, w->windowCreationInfo.colorDepth.g);
                    glfwWindowHint(GLFW_BLUE_BITS,  w->windowCreationInfo.colorDepth.b);
                    glfwWindowHint(GLFW_ALPHA_BITS, w->windowCreationInfo.colorDepth.a);

                    glfwWindowHint(GLFW_DEPTH_BITS, w->windowCreationInfo.depthBits);
                    glfwWindowHint(GLFW_STENCIL_BITS, w->windowCreationInfo.stencilBits);

                    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

                    int major, int minor; bool coreProfile;
                    switch (w->app->config.opengl_version) {
                        case OpenGLVersion::v21: major = 2, minor = 1, coreProfile = false; break;
                        case OpenGLVersion::v32: major = 3, minor = 2, coreProfile = true; break;
                        case OpenGLVersion::v41: major = 4, minor = 1; coreProfile = true; break;
                        case OpenGLVersion::v45: major = 4, minor = 5; coreProfile = true; break;
                        default: assert("Invalid opengl version!");
                    }

                    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
                    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
                    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, !coreProfile);
                    glfwWindowHint(GLFW_OPENGL_PROFILE, coreProfile ? GLFW_OPENGL_CORE_PROFILE : GLFW_OPENGL_COMPAT_PROFILE);

                    // I think that's all of the relevant options -- see http://www.glfw.org/docs/3.0/window.html#window_hints

                    w->window = glfwCreateWindow(
                        w->properties.size.x, w->properties.size.y,     // window dimensions
                        w->properties.title.c_str(),                    // window title
                        w->getGLFWMonitorFromProperties(),              // monitor
                        w->getPrimaryActiveMonitor(),                   // get active monitor (to share resources)
                    );
                    if (w->window == nullptr) {
                        throw new GlfwException("Could not create window '" + w->name + "'");
                    }

                    glfwSetWindowUserPointer(w->window, w);

                    w->updateWindowSizeInfo();

                    glfwSetWindowCloseCallback(w->window, onWindowCloseCallback);
                    glfwSetWindowSizeCallback(w->window, onWindowSizeCallback);
                    glfwSetFramebufferSizeCallback(w->window, onFramebufferSizeCallback);
                    glfwSetWindowPosCallback(w->window, onWindowPosCallback);

                    glfwSetKeyCallback(w->window, onKeyCallback);
                    glfwSetCharCallback(w->window, onCharCallback);
                    glfwSetMouseButtonCallback(w->window, onMouseButtonCallback);
                    glfwSetCursorPosCallabck(w->window, onCursorPosCallback);
                    glfwSetCursorEnterCallback(w->window, onCursorEnterCallback);
                    glfwSetScrollCallback(w->window, onScrollCallback);

                    context.notifyEvent<Event::WindowCreated>(window);
                }
            }
        };
        struct Destroy {
            std::weak_ptr<AppWindow> window;
            enum { id = AppCommand::kDestroyWindow, EXEC_TARGET = MAIN_THREAD_ONLY };
            void execute (AppContext& context) {}
        };
        struct SetPos {
            std::weak_ptr<AppWindow> window;
            glm::vec2                pos;

            enum { id = AppCommand::kSetWindowPos, EXEC_TARGET = MAIN_THREAD_ONLY };
            void execute (AppContext& context) {
                if (auto w = window.lock()) {
                    if (w->pos != pos) {
                        context.notifyEvent<Event::WindowPosChanged>(window, w->pos, pos);
                        w->pos = pos;
                        if (w->window) glfwSetWindowPos(w->window, pos.x, pos.y);
                    }
                }
            }
        };
    }; // class AppWindow
} // namespace backend

namespace frontend {

// Window data model
struct WindowData {
    glm::vec2 pos, size, scale;
    std::string title;
};

// Window facade (get / set methods to access data + backend)
class AppWindow {
    #define DEFN_PROPERTY(property, setOp) \
    class Property_##property { \
        auto outer () { return reinterpret_cast<AppWindow*>(static_cast<void*>(this) - offsetof(AppWindow, property)); } \
        public: \
        const auto& get () { return outer()->data.property; } \
        void set (const decltype(get())& v) { \
            if (v != outer()->data.property) \
                outer()->backend.setOp(outer()->data.property = v); \
        } \
    } property;

    std::shared_ptr<backend::AppWindow> backend;
    WindowData                          data;

    // DEFN_PROPERTY(prop, setter) => Defines anonymous class + 1 instance w/ methods:
    //  prop.get():      returns data.prop
    //  prop.set(v):     calls backend->setter(data.prop = v);

    DEFN_PROPERTY(pos,      setPos);
    DEFN_PROPERTY(size,     setSize);
    DEFN_PROPERTY(scale,    setScale);
    DEFN_PROPERTY(title,    setTitle);

    // DEFN_PROPERTY(pos, backend::AppWindow::SetPos);
    // DEFN_PROPERTY(size, backend::AppWindow::SetSize);
    // DEFN_PROPERTY(scale, backend::AppWindow::SetScale);
    // DEFN_PROPERTY(title, backend::AppWindow::SetTitle);
    #undef DEFN_PROPERTY
};

} // namespace


































