
#pragma once
#include "concurrentqueue.h"
#include <thread>
#include <atomic>

template <typename Impl>
class KThread {
    using moodycamel::ConcurrentQueue;
    typedef Impl::ThreadEvent ThreadEvent;
    friend class KThreadRunner;
public:
    KThread () {}
    KThread (const KThread&) = delete;
    KThread& operator= (const KThread&) = delete;
    virtual ~KThread () {}

    void kill       () { m_keepRunning = false; }
    bool isRunning  () const { return m_isRunning; }
    void exec       (const ThreadEvent& ev) {
        eventQueue.enqueue(ev);
    }
    static KThread* mainThread () { return g_mainThread; }
    static KThread* glThread   () { return g_glThread;   }
protected:
    template <typename F>
    bool tryExec (const F& fcn) {
        try {
            fcn();
            return true;
        } catch (const std::exception& e) {
            static_cast<Impl*>(this)->onThreadException(e);
        } catch {
            static_cast<Impl*>(this)->onThreadException(
                std::runtime_error("Unknown exception"));
        }
        return false;
    }
    void start (std::shared_ptr<KThread> selfRef) {
        m_isRunning = true;
        m_keepRunning = true;
        tryExec([&this](){
            static_cast<Impl*>(this)->onThreadBegin();
            run();
        });
        tryExec([&this](){
            static_cast<Impl*>(this)->onThreadEnd();
        });
        m_isRunning = false;
    }
    void run () {
        Impl::ThreadEvent ev;
        while (m_keepRunning) {
            tryExec([&this](){
                if (static_cast<Impl*>(this)->maybeUpdate()) {}
                else if (m_eventQueue.try_dequeue(ev)) { ev(); }
                else {
                    static_cast<Impl*>(this->onQueueEmpty());
                }
            });
        }
    }
private:
    ConcurrentQueue<ThreadEvent>  m_eventQueue;
    std::atomic<bool>             m_keepRunning = false;
    std::atomic<bool>             m_isRunning   = false;

    static KThread* g_mainThread = nullptr;
    static KThread* g_glThread   = nullptr;
};

class KThreadRunner {
    KThreadRunner (KThread* worker) : m_thread(worker) {
        m_thread->start(m_thread);
    }
    KThreadRunner (KThread* worker, std::thread thread) : m_thread()
    ~KThreadRunner () {
        m_thread->kill();
    }
private:
    std::shared_ptr<KThread> m_worker;
};

class MainThread : public KThread<MainThread> {
public:
    typedef std::function<void()> ThreadEvent;

    void onThreadBegin () {}
    void onThreadEnd   () {}
    void onThreadException (const std::exception& e) {

    }
    bool maybeUpdate () { 
        // Maybe do stuff...?

        return false; 
    }
    void onQueueEmpty () {
        // wait...?
    }
};
class GLThread : public KThread<GLThread> {
public:
    typedef std::function<void()> ThreadEvent;

    void onThreadBegin () {}
    void onThreadEnd   () {}
    void onThreadException (const std::exception& e) {

    }
    bool maybeUpdate () { 
        // Maybe do stuff...?

        return false; 
    }
    void onQueueEmpty () {
        // wait...?
    }
};

class KThreadRunner {
    std::shared_ptr<KThread> mt, gt;
public:
    KThreadRunner () {
        assert(KThread::g_mainThread == nullptr);
        KThread::g_mainThread = new MainThread();
        mt = std::shared_ptr<KThread>(KThread::g_mainThread); 
        assert(KThread::g_mainThread == mt.ptr());

        assert(KThread::g_glThread == nullptr);
        KThread::g_glThread = new GLThread();
        mt = std::shared_ptr<KThread>(KThread::g_glThread); 
        assert(KThread::g_glThread == gt.ptr());
    }
    ~KThreadRunner () {
        mt->kill(); gt->kill();

        assert(KThread::g_mainThread != nullptr);
        KThread::g_mainThread = nullptr;

        assert(KThread::g_glThread != nullptr);
        KThread::g_glThread = nullptr;
    }
    void enterMainThread () { mt->start(mt); }
    void enterGLThread   () { gt->start(gt); }
};

