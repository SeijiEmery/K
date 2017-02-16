
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
// Macros to define user command buffer operations + linkage. See usage at end.
//
// To implement a Command or Event data structure:
// – define an enum class w/ unique entries for all values, starting at 1.
// – must define a special value of NONE = 0, as mentioned below
// – define structs w/ command / event data
// – commands must have a void execute () method
//
// map the struct types to enum values w/:
// – DEFINE_COMMAND(E::VALUE, T) for commands / events
// – DISPATCH_COMMAND(E::VALUE, T) inside a BEGIN / END block for commands
// – DISPATCH_VISITOR(E::VALUE, T) inside a BEGIN / END block for events
//
// To visit events:
// – define a Visitor struct inheriting from CommandBuffer<E>::Visitor<YourVisitorClass>.
// – define 'void visit (T&)' methods for all event types. Missing methods => compile errors.
// – invoke your visitor's methods w/ <yourVisitorInstance>.visit(CommandBuffer<E>& instance)
//
// Note:
// – structs may double as both Commands and Events
// – structs may be reused as backing data type for multiple different command buffers;
//   enum classes cannot.
// – any type can be used to implement events: existing classes / structs, int, etc.
// – However, using non-POD types will result in undefined behavior atm; values are assigned
//   to / from for single access, but if a CommandBuffer is copied it does so via memcpy().
// – Implementing proper copy-construction for command elements would needlessly complicate
//   the structure of CommandBuffer, so please use POD types + non-owning pointers to implement
//   commands + events atm. (note: would memcpying std::weak_ptr<T> work?).
//
// See examples at the end for exact usage and expected patterns.
//


// Defines a CommandBuffer dispatch method for a given CMD (enum class) type.
//
// CMD_ENUM *must* define CMD_ENUM::NONE = 0, which should not overlap with any other value.
// – CMD_ENUM::NONE is a special value.
// – It is illegal to define operations on CMD_ENUM::NONE, eg.
//      DEFINE_COMMAND(<CMD_ENUM>::NONE, ...)
//  or  DISPATCH_COMMAND(<CMD_ENUM>::NONE, ...)
//  or  DISPATCH_VISITOR(<CMD_ENUM>::NONE, ...)
//
#define BEGIN_COMMAND_DISPATCH_IMPL(CMD_ENUM) \
void dispatch (CommandBuffer<CMD>& buffer) {\
    cb.rewindReadHead(); \
    bool done = false; \
    E command; \
    while (!done) { \
        switch (CMD command = buffer.readNext()) { \
            case CMD::NONE: done = true; break;

// End CommandBuffer dispatch method
#define END_COMMAND_DISPATCH_IMPL \
            default: static_assert(0, "Missing command(s)"); \
        } \
    } \
}

// "Connect" an enum value (must be an element of CMD), and command structure type
// (must have a void execute() method). Must occur between BEGIN / END _COMAND_DISPATCH_IMPL.
#define DISPATCH_COMMAND(CMD,T) \
    case CMD: cb.read<T>->execute(); break;


// Defines a CommandBuffer write method, "Connecting" an enum value and command structure type 
// (must have a void execute() method). This is a freestanding declaration, bust must be matched
// with a DISPATCH_COMMAND statement between BEGIN / END _COMMAND_DISPATCH_IMPL statements.
#define DEFINE_COMMAND(CMD,T) \
auto& write (CommandBuffer<decltype(E)>& buffer, const T& data) { \
    return buffer.write(CMD, data), buffer; \
}

#define BEGIN_COMMAND_VISIT_IMPL(CMD_ENUM) \
template <typename Visitor> \
void visit (CommandBuffer<CMD_ENUM> cb, Visitor& visitor) { \
    cb.rewindReadHead(); \
    bool done = false; \
    E command; \
    while (!done) { \
        switch (CMD_ENUM command = buffer.readNext()) { \
            case CMD_ENUM::NONE: done = true; break;

#define DISPATCH_VISITOR(CMD,T) \
    case CMD: visitor.visit(*cb.read<T>); break;

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
    struct Baz { ... void execute();  };
    typedef CommandBuffer<kExampleCmd> CommandBuffer;
}

DEFINE_COMMAND(kExampleCmd::FOO, ExampleCommand::Foo)
DEFINE_COMMAND(kExampleCmd::BAR, ExampleCommand::Bar)
DEFINE_COMMAND(kExampleCmd::BAZ, ExampleCommand::Baz)

BEGIN_COMMAND_DISPATCH_IMPL(kExampleCmd)
    DISPATCH_COMMAND(kExampleCmd::FOO, ExampleCommand::Foo)
    DISPATCH_COMMAND(kExampleCmd::BAR, ExampleCommand::Bar)
    DISPATCH_COMMAND(kExampleCmd::BAZ, ExampleCommand::Baz)
END_COMMAND_DISPATCH_IMPL


// Command implementation (source file)
void ExampleCommand::Foo::execute () { ... }
void ExampleCommand::Bar::execute () { ... }
void ExampleCommand::Baz::execute () { ... }

//
// Event usage:
//

void pushEvents (ExampleCommand::CommandBuffer& cb) {
    cb.write(ExampleCommand::Foo{ ... });
    cb.write(ExampleCommand::Foo{ ... });
    cb.write(ExampleCommand::Baz{ ... });
}
void runEvents (ExampleCommand::CommandBuffer& cb) {
    dispatch(cb);
}


//
// Event Example
//

// Event definition (header file)
enum class kExampleEvent { NONE = 0, FOO, BAR, BAZ };
namespace ExampleEvent {
    struct Foo { ... };
    struct Bar { ... };
    struct Baz { ... };
    typedef CommandBuffer<kExampleEvent> CommandBuffer;
}

DEFINE_COMMAND(kExampleEvent::FOO, ExampleEvent::Foo)
DEFINE_COMMAND(kExampleEvent::BAR, ExampleEvent::Bar)
DEFINE_COMMAND(kExampleEvent::BAZ, ExampleEvent::Baz)

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

    MyVisitor& visit (ExampleEvent::CommandBuffer& events) {
        return visit(events, *this), *this;
    }
};

void visitExample (ExampleEvent::CommandBuffer& events) {
    MyVisitor visitor { ... };
    visitor.visit(events);
}

// OR

struct MyVisitor : public ExampleEvent::CommandBuffer::Visitor<MyVisitor> {
    ...
    MyVisitor (...) : ... {}

    void visit (ExampleEvent::Foo& ev) { ... }
    void visit (ExampleEvent::Bar& ev) { ... }
    void visit (ExampleEvent::Baz& ev) { ... }

    // Automatic visit() method + typedefs for:
    // – CommandType    (enum class type)
    // – CommandBuffer  (CommandBuffer<Command>)
};

void visitExample (MyVisitor::CommandBuffer& events) {
    MyVisitor visitor { ... };
    visitor.visit(events);
}

// OR

void visitExample (CommandBuffer<MyVisitor::CommandType>& events) {
    MyVisitor visitor { ... };
    visitor.visit(events);   
}

#endif

