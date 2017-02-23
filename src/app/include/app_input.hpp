
#pragma once

namespace k {
namespace app {
namespace input {

//
// Basic interfaces for accessing input state.
// Written with a C-ish interface so these can be used portably if/when needed;
// as such, we're using fixed-size arrays and not std::vector / std::array.
//


// Data structures defined in this file:
struct MouseState;      // holds current + prev states for mouse + brief event history for last several seconds
struct KeyboardState;   // holds current + prev states for keyboard + brief event history for last several seconds
struct GamepadState;    // holds current + prev states for gamepad + brief event history for last several seconds
struct InputState;      // holds 1 MouseState, 1 KeyboardState, 0+ GamepadStates, and brief connection history


struct PressEvent {  // 12 bytes
    uint16_t button;                // button index
    uint16_t pressCount;            // # of recorded presses (double click = 2, released = 0)
    double   pressTime;             // Seconds since pressed / released

    struct Iterator;
};

namespace detail {

//
// Utility functions for dealing w/ pressState.
//

template <typename T>
PressEvent* getNextPressEvent (PressState* current, const T& state, decltype(state.pressEvents[0].button) button) {
    if (current == nullptr || current->button != button) {
        current = &state.pressEvents[0];
    } else {
        // assert(current >= &state.pressEvents[0] && current < &state.pressEvents[state.numPressEvents]);
        ptrdiff_t offset = current - state.pressEvents[0];
        if (offset < 0 || offset >= state.numPressEvents)
            throw new std::runtime_exception("Invalid pointer passed to getNextPressEvent()!");
        ++current;
    }
    for (; current < &state.pressEvents[state.numPressEvents]; ++current)
        if (current->button == button)
            return current;
    return nullptr;
}

template <typename T>
PressEvent* getPrevPressEvent (PressState* current, const T& state, decltype(state.pressEvents[0].button) button) {
    if (current == nullptr || current->button != button) {
        current = &state.pressEvents[state.numPressEvents-1];
    } else {
        // assert(current >= &state.pressEvents[0] && current < &state.pressEvents[state.numPressEvents]);
        ptrdiff_t offset = current - state.pressEvents[0];
        if (offset < 0 || offset >= state.numPressEvents)
            throw new std::runtime_exception("Invalid pointer passed to getNextPressEvent()!");
        --current;
    }
    for (; current < &state.pressEvents[]; --current)
        if (current->button == button)
            return current;
    return nullptr;
}

} // namespace detail

// Not a real iterator, but close enough?
#define DEFN_PRESS_STATE_METHODS \
    PressState* firstPressEvent (decltype(button) button) const { return getNextPressEvent(nullptr, *this, button); } \
    PressState* lastPressEvent  (decltype(button) button) const { return getPrevPressEvent(nullptr, *this, button); } \
    PressState* nextPressEvent  (PressState* ev, decltype(button) button) const { return ev ? getNextPressEvent(ev, *this, button) : nullptr; } \
    PressState* prevPressEvent  (PressState* ev, decltype(button) button) const { return ev ? getPrevPressEvent(ev, *this, button) : nullptr; } \


struct MouseState {  // 448 bytes
    static const size_t NUM_BUTTONS = 8;

    // 32 bytes
    glm::vec2d pos,     prevPos;        // Mouse position, in pixels
    glm::vec2d scroll,  prevScroll;     // Scroll *delta*, in pixels.

    // Scroll coordinates:
    //  Scroll.y = vertical (default)       +y: up,    -y: down
    //  Scroll.x = horizontal (if 2-axis)   +x: right, -x: left
    //  Note: for 1d UI controls, either max(.x, .y), (.y || .x), or .y is usually a safe bet.

    // 16 bytes
    uint8_t[NUM_BUTTONS] pressCount;
    uint8_t[NUM_BUTTONS] prevPressCount;

    // 16 bytes
    size_t numPressEvents;
    size_t __padding;

    // 384 bytes
    static const size_t MAX_PRESS_EVENTS = 32;
    PressEvent[MAX_PRESS_EVENTS] pressEvents;

    // Utility methods:

    DEFN_PRESS_STATE_METHODS
};



struct KeyboardState {  // 472 bytes
    static const size_t NUM_KEYS = 352;       // Rounded up from GLFW_NUM_KEYS (348); is evenly divisible by 8.

    // 88 bytes
    uint8_t[NUM_KEYS / 8] keyPressBitmask;
    uint8_t[NUM_KEYS / 8] prevKeyPressBitmask;

    size_t numPressEvents;

    // 384 bytes
    static const size_t MAX_PRESS_EVENTS = 32;
    PressEvent[MAX_PRESS_EVENTS] pressEvents;

    // Utility methods:

    DEFN_PRESS_STATE_METHODS
};

//
// Note: we distinguish between gamepads (which have a very specific, well-defined layout),
// and other joystick devices (which could be anything, but encompass flight sticks, etc).
//
// Our approach is as follows: Generally speaking, dealing with multiple gamepad types is / can
// be a PITA due to different axes / button bindings, etc. So, we handle this within at the
// library level and map all input to a generic gamepad model with a well-defined interface.
//
// This has 2 layers:
// – a) Gamepad mapper classes, which convert input from one input type (eg. DirectInput / DS3 / DS4
// input bindings, etc) to our generic model.
// - b) User configuration: what device configurations get mapped to which model, and the specifics of:
//      - deadzones + analog input conversion (eg. non-linear?)
//      - rebinding buttons + axes, flipping axes, etc., on top of the input converter.
//
// Under our implementation, all gamepads have:
//  – 4 face buttons (A/B/X/Y)
//  – DPAD, treated as both 2 axes + individual buttons
//  – 2 analogue sticks
//  – 2 individual bumpers
//  – 2 individual triggers, treated as both axes + buttons (threshold is user-configurable)
//  – start / select / home buttons
//  – 
//  – any missing hardware features will be ignored (button(s) always display non-pressed, axes 0, etc)
//  – any additional hardware features available via extensions, see below:
//
// NEW INPUT MODELS:
//  – If / when necessary, can add additional input models, eg. flight stick, motion controllers
//  for occulus / vive, etc.
//  – It does not make sense to treat these as gamepads, so they will have 
//  – But! We can still allow for input emulation (eg. flight stick => gamepad); in that case
//  one hardware device could simply be bound / represented as multiple things. For starters,
//  we could use this to build an emulated mouse + keyboard gamepad, for instance.
//  – This is so stuff like game controls + UI will always be designed to work with ONE thing,
//  eg. 3 UI implementations: mouse + keyboard, xbox-like gamepad, HTC vive; we'd have support for
//  many other devices though by mapping those onto these 3 implementations (or 2, or whatever).
//
// GAMEPAD EXTENSIONS:
// – To add support for eg. DS4 special features -- ie. trackpad, light color, motion tracking, etc.,
// use the following approach:
//
// struct DS4GamepadState {
//      GamepadState gamepad;
//      // new state goes here (touch state, light state, etc)
// };
//
// User code:
//      for (GamepadState* g = inputState->gamepad; g; g = g->next) {   // Iterate gamepads
//          if (g->flags & gamepad::TYPE_DS4) {                         // Check for DS4 type flag
//              DS4GamepadState* ds4 = (DS4GamepadState*)(g);           // Cast to extended data structure
//              // do stuff w/ DS4 properties...
//          }
//      }
//
// Note: We could use c++ inheritance, but this is backwards-compatible with C (ie. portable),
// and keeps the interface quite simple; however, this only supports single inheritance ofc,
// so these extensions should never be deeply nested.
//
// TLDR;
// To add support for new gamepad features, extend Gamepad (is backwards compatible w/ existing controls + UI).
// To add support for new input paradigms (eg. VR), write a new input model. (must write new controls + UI).
// One physical device can be mapped to multiple models, and can exist in multiple places.
//
// This is important; user code should never have to figure out whether a gamepad is a flight stick or not,
// and vice versa. To add support for new input devices, write new input handlers + UI.
//

namespace gamepad {

enum class Button {
    A = 0, B, X, Y,         // on DS3/DS4 => x / circle / square / triangle
    DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT,
    LTRIGGER, RTRIGGER, LBUMPER, RBUMPER,
    LSTICK, RSTICK, START, SELECT, HOME, 
    LAST = HOME
};
enum class Axis {
    LX = 0, LY,             // Left stick inputs,  each [-1, 1]
    RX, RY,                 // Right stick inputs, each [-1, 1]
    LTRIGGER, RTRIGGER,     // Individual left / right trigger inputs, each [0, 1]
    DPAD_X, DPAD_Y,         // Dpad inputs,        each [-1, 1]
    LAST = DPAD_Y
};
enum {  
    NUM_BUTTONS = Button::LAST + 1,
    NUM_AXES    = Axis::LAST + 1,
}

// Gamepad identification flags to differentiate between known feature sets + signal extensions.
// Extend as needed.
enum {
    TYPE_INVALID = 0,       // invalid; should never have this type.
    TYPE_UNKNOWN = 1,       // unknown xbox-like type. Signals the programmer/user that we haven't added full support for this yet.
    TYPE_XBOX    = 2,       // xbox / 360 / one controller
    TYPE_DS3     = 3,       // PS3/2/1 controller
    TYPE_DS4     = 4,       // PS4 controller (touchpad + glowbar unsupported, but could add support later)

    IS_PS_LIKE   = TYPE_DS3 | TYPE_DS4;     // Should display playstation UI, etc.
    IS_XBOX_LIKE = ~IS_PS_LIKE;             // Should display xbox UI, etc.
};

} // namespace gamepad

struct GamepadState {
    uint32_t id;            // Unique id for this gamepad, kept through application scope
    uint32_t flags;         // Type flags
    GamepadState* next;     // Non-owning pointer to next active gamepad. List is kept sorted by id, lowest 1st.
    const char*   hidName;  // Non-owning c-string w/ device name
    size_t numPressEvents;

    static const size_t NUM_BUTTONS = gamepad::NUM_BUTTONS;
    static const size_t NUM_AXES    = gamepad::NUM_AXES;

    float[NUM_AXES]      axes,          prevAxes;       // Normalized axis states (see gamepad::Axis)
    uint8_t[NUM_BUTTONS] pressCount,    prevPressCount; // Press counts for each gamepad::Button.

    static const size_t MAX_PRESS_EVENTS = 32;
    PressEvent[MAX_PRESS_EVENTS]        pressEvents;

    // Utility methods:

    DEFN_PRESS_STATE_METHODS
};

//
// Input state (separate copy passed to each AppClient each frame; if paused, will update accordingly).
//

struct InputState {
    MouseState    mouse;
    KeyboardState keyboard;
    GamepadState* gamepads;  // non-owning list of all active gamepads, sorted by id (lowest 1st).

    // To iterate + handle input on gamepads:
    // for (GamepadState* g = inputState->gamepads; g; g = g->next) {
    //      auto id     = g->id;
    //      auto flags  = g->flags;
    //      bool justPressedLT = (g->pressState[gamepad::Axis::LT] != 0) && (g->prevPressState[gamepad::Axis::LT] == 0);
    //      if (justPressedLT) {
    //          unsigned LT_pressCount = g->pressState[gamepad::Axis::LT];
    //          double   LT_timeSincePressed = 0;
    //          if (auto ev = g->firstPressEvent(gamepad::Axis::LT))
    //              LT_timeSincePressed = ev->pressTime;
    //      }
    // }
};

}
}
}
