
#pragma once

template <size_t CHUNK_SIZE = 4096 * 4096>
class ChunkedForwardList {
    constexpr SIZE = CHUNK_SIZE - sizeof(size_t) * 2;
    struct Chunk {
        Chunk* next; size_t head = 0;
        uint8_t data[SIZE];

        Chunk () { std::fill(&data[0], &data[SIZE], 0); }
        Chunk (const Chunk& chunk) {
            memcpy(static_cast<void*>(this), static_cast<void*>(&chunk), sizeof(Chunk));
            next = chunk->next ?
                new Chunk(*chunk->next) :
                nullptr;
        }

        void rewind () { head = 0; }
        void clear ()  { head = 0; memcpy(static_cast<void*>(&chunk->data[0]), 0, SIZE); }

        template <typename T>
        void write (const T& value, Chunk*& head) {
            static_assert(sizeof(T) < SIZE);
            if (SIZE - head <= sizeof(T)) {
                if (!next) next = new Chunk();
                head = next;
                next->write(value);
            }
            *static_cast<T*>(&data[head]) = value;
            head += sizeof(T);
        }

        template <typename T>
        const T* read (Chunk*& head) {
            static_assert(sizeof(T) < SIZE);
            if (SIZE - head <= sizeof(T)) {
                if (!next) return nullptr;
                head = next;
                return next->read(value);
            }
            auto ptr = static_cast<T*>(&data[head]);
            head += sizeof(T);
            return *ptr;
        }
        ~Chunk () { if (head) delete head; }
    };
    Chunk* head;
    size_t unused;
    Chunk first;

    void resetHead () {
        head = &first;
        for (auto seg = head; seg; seg = seg->next)
            seg->rewind();
    }
    void clear () {
        head = &first;
        for (auto seg = head; seg; seg = seg->next)
            seg->clear();
    }

    template <typename T>
    void write (const T& value) {
        head->write(value, head);
    }
    template <typename T>
    const T* value read () {
        return head->read(head);
    }

    ChunkedForwardList () : head(&first) {}
    ChunkedForwardList (const ChunkedForwardList &cl) 
        : head(nullptr), 
          first(cl.first)    // copy chunk data recursively via Chunk ctor
    {
        // Iterate forward until we find the node that cl.head points to.
        // Set the matching node in our list as our head.
        for (auto src = &cl.first, dst = &first; src; src = src->next, dst = dst->next) {
            assert((src != nullptr) == (dst != nullptr));
            if (src == cl.head) 
                head = dst;
        }
        assert(head != nullptr);
    }
};

template <class CommandType>
class CommandBuffer {
    std::unique_ptr<ChunkedForwardList> buffer;
    typedef CommandType CommandType;

    CommandBuffer () : buffer(new decltype(buffer)());
    CommandBuffer (CommandBuffer&& cb) : buffer(std::move(cb.buffer)) {}
    CommandBuffer (const CommandBuffer& cb) : buffer(new decltype(buffer)(cb.buffer)) {}
    virtual ~CommandBuffer () {}

    void rewindReadHead () { buffer->resetHead(); }
    void clear () { buffer->clear(); }

    template <typename T>
    void write (CommandType command, const T& data) {
        buffer->write(command);
        buffer->write(data);
    }
    CommandType readNext () {
        auto ptr = buffer->read<CommandType>();
        return ptr ? *ptr : CommandType::NONE;    // end of stream gets translated to command '0'.
    }
    template <typename T>
    const T* read () {
        return buffer->read<T>();
    }

    // Base Visitor CRTP class template. Inherit from this to provide a default visit() method.
    template <typename Self>
    struct Visitor {
        typedef CommandType CommandType;
        typedef CommandBuffer<CommandType> CommandBuffer;

        Self& visit (CommandBuffer& buffer) {
            return visit(*static_cast<Self*>(this), buffer), *static_cast<Self*>(this);
        }
    };
};

//
// Macros to define user command buffers. See usage at end.
//
// If implemented, defines write(CommandBuffer<K>, const V& commandValue),
// and dispatch(CommandBuffer<K>), which calls V::execute() for all matching V types.
//

// Defines a CommandBuffer dispatch method for a given CMD (enum class) type.
//
// CMD *must* define CMD::NONE = 0, which should not overlap with any other value.
// Note: this is a special value and defining DEFINE_COMMAND(CMD::NONE, ...) 
// or DISPATCH_COMMAND(CMD::NONE, ...) is illegal.
//
#define BEGIN_COMMAND_DISPATCH_IMPL(CMD) \
void dispatch (CommandBuffer<CMD>& buffer) {\
    cb.rewindReadHead(); \
    bool done = false; \
    E command; \
    while (!done) { \
        switch (CMD command = buffer.readNext()) { \
            case CMD::NONE: atEnd = true; break;

// End CommandBuffer dispatch method
#define END_COMMAND_DISPATCH_IMPL \
            default: static_assert(0, "Missing command(s)"); \
        } \
    } \
}

// "Connect" an enum value (must be an element of CMD), and command structure type
// (must have a void execute() method). Must occur between BEGIN / END _COMAND_DISPATCH_IMPL.
#define DISPATCH_COMMAND(EV,T) \
    case EV: cb.read<T>->execute(); break;


// Defines a CommandBuffer write method, "Connecting" an enum value and command structure type 
// (must have a void execute() method). This is a freestanding declaration, bust must be matched
// with a DISPATCH_COMMAND statement between BEGIN / END _COMMAND_DISPATCH_IMPL statements.
#define DEFINE_COMMAND(E,T) \
auto& write (CommandBuffer<decltype(E)>& buffer, const T& data) { \
    return buffer.write(E, data), buffer; \
}

#define BEGIN_COMMAND_VISIT_IMPL(CMD) \
template <typename Visitor> \
void visit (CommandBuffer<CMD> cb, Visitor& visitor) { \
    cb.rewindReadHead(); \
    bool done = false; \
    E command; \
    while (!done) { \
        switch (CMD command = buffer.readNext()) { \
            case CMD::NONE: atEnd = true; break;

#define DISPATCH_VISITOR(EV,T) \
    case EV: visitor.visit(*cb.read<T>); break;

#define END_COMMAND_VISIT_IMPL END_COMMAND_DISPATCH_IMPL

//
// Macro usage / example:
//

#if 0

//
// Command Example
//

// Command definition (header file)
enum class kExampleCmd { NONE = 0, FOO, BAR, BAZ };
namespace ExampleCommand {
    struct Foo { ... void execute (); };
    struct Bar { ... void execute (); };
    struct Baz { ... void execute(); };
}

BEGIN_COMMAND_DISPATCH_IMPL(kExampleCmd)
    DISPATCH_COMMAND(kExampleCmd::FOO, ExampleCommand::Foo)
    DISPATCH_COMMAND(kExampleCmd::BAR, ExampleCommand::Bar)
    DISPATCH_COMMAND(kExampleCmd::BAZ, ExampleCommand::Baz)
END_COMMAND_DISPATCH_IMPL

DEFINE_COMMAND(kExampleCmd::FOO, ExampleCommand::Foo)
DEFINE_COMMAND(kExampleCmd::BAR, ExampleCommand::Bar)
DEFINE_COMMAND(kExampleCmd::BAZ, ExampleCommand::Baz)

// Command implementation (source file)
void ExampleCommand::Foo::execute () { ... }
void ExampleCommand::Bar::execute () { ... }
void ExampleCommand::Baz::execute () { ... }

//
// Event Example
//

// Event definition (header file)
enum class kExampleEvent { NONE = 0, FOO, BAR, BAZ };
namespace ExampleEvent {
    struct Foo { ... };
    struct Bar { ... };
    struct Baz { ... };
}

BEGIN_COMMAND_VISIT_IMPL(kExampleEvent)
    DISPATCH_VISITOR(kExampleEvent::FOO, ExampleEvent::Foo)
    DISPATCH_VISITOR(kExampleEvent::BAR, ExampleEvent::Bar)
    DISPATCH_VISITOR(kExampleEvent::BAZ, ExampleEvent::Baz)
END_COMMAND_VISIT_IMPL

// Event usage (source file)

struct MyVisitor {
    ...
    MyVisitor (...) : ... {}

    void visit (ExampleEvent::Foo& ev) { ... }
    void visit (ExampleEvent::Bar& ev) { ... }
    void visit (ExampleEvent::Baz& ev) { ... }

    MyVisitor& visit (CommandBuffer<kExampleEvent>& events) {
        return visit(events, *this), *this;
    }
}

void visitExample (CommandBuffer<kExampleEvent>& events) {
    MyVisitor visitor { ... };
    visitor.visit(events);
}

// OR

struct MyVisitor : public CommandBuffer<kExampleEvent>::Visitor<MyVisitor> {
    ...
    MyVisitor (...) : ... {}

    void visit (ExampleEvent::Foo& ev) { ... }
    void visit (ExampleEvent::Bar& ev) { ... }
    void visit (ExampleEvent::Baz& ev) { ... }

    // Automatic visit() method + typedefs for:
    // – Command        (enum class type)
    // – CommandBuffer  (CommandBuffer<Command>)
}

void visitExample (MyVisitor::CommandBuffer& events) {
    MyVisitor visitor { ... };
    visitor.visit(events);
}

// OR

void visitExample (CommandBuffer<MyVisitor::Command>& events) {
    MyVisitor visitor { ... };
    visitor.visit(events);   
}

#endif

