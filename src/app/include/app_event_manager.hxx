
#pragma once

namespace k {
namespace app {

//
//  Really basic, unoptimized event system.
//  TODO: Tests, thread safety (perhaps thread scheduling...?)
//

namespace event {

struct BaseListener {
    BaseListener* next; // pointer to next listener
    bool  active;       // make this atomic?

    virtual ~BaseListener () {
        if (next) delete next;
    }
};

template <typename... Args>
struct EventListener : public BaseListener {
    std::function<void(Args...)> callback;

    EventListener (decltype(callback) callback, BaseListener* next) :
        next(next), active(true), callback(callback) {}
};

}; // namespace event

// Must inherit from this, or provide an instance of this, to use the event system.
// Stores a list of connected events and detaches them on dtor.
class EventAnchor {
private:
    using namespace event;
    std::vector<BaseListener*> ownedListeners;
public:
    virtual ~EventAnchor () {
        for (auto listener : connectedEvents)
            listener->active = false;
    }
    void addOwnedListener (BaseListener* listener) {
        ownedListeners.push_back(listener);
    }
};


// Event dispatcher.
template <typename... Args>
class Event {
    using namespace event;

    EventListener<Args...>* listener;
    ~Event () { if (listener) delete listener; }

    void operator () (Args... args) {
        auto l = listener, **lp = &listener;
        while (l) {
            if (!l->active) { 
                lp->next = l->next;
            } else {
                lp->callback(args...);
            }
            lp = &l;
            l  = static_cast<decltype(l)>(l->next);
        }
    }
    void connect (EventAnchor& anchor, std::function<void(Args...)> callback) {

        // Currently just malloc-ing listener nodes w/ malloc.
        // Could optimize w/ a memory pool / custom allocator or something...
        listener = new Listener(callback, listener);
        anchor.addOwnedListener(listener);
    }

    template <typename AnchorDescendent>
    void connect (AnchorDescendent& anchor, std::function<void(Args...)> callback) {
        connect(static_cast<EventAnchor&>(anchor), callback);
    }
    template <typename AnchorDescendent>
    void connect (AnchorDescendent& anchor, void(*callback)(Args...)) {
        connect(static_cast<EventAnchor&>(anchor), { callback });
    }
    template <typename AnchorDescendent, Method>
    void connect (AnchorDescendent& anchor, Method* method) {
        connect(static_cast<EventAnchor&>(anchor), [&](Args... args){
            anchor->*method(args...);
        });
    }
};

class EventManager {};

}; // namespace app
}; // namespace k
