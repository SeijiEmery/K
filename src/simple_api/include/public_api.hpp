
#pragma once

// Internal class
class AppManager;

// Public state, w/ ability to handle window creation + query input states (immutable + double buffered).
// For simplicity, our public API (this struct) is implemented using PODs.
class ApplicationState;

//
// Window + screen sate
//

// POD screen type, representing a physical screen / monitor.
// This is a read-only data structure; both the screen reference and screen list are hidden behind const ptrs.
struct Screen {
    unsigned        id;
    glm::vec2i      resolution;
    double          dpiScale;

    typedef std::shared_ptr<const Screen> ConstPtr;
};
typedef std::vector<Screen::ConstPtr> ScreenList;


// POD Window struct -- represents a read/write reference to a native window (implementation hidden).
// All fields are public read / write, and are synchronized to an internal window object.
//
// To create a window, add it to the window list; to destroy it, remove it. Window identity
// is handled via using shared_ptr.
//
struct Window {
    Screen::ConstPtr screen;
    std::string     title;
    glm::vec2i      size;
    glm::vec2i      pos;
    double          dpiScale = 1.0;
    bool            visible  = false;
};
typedef std::vector<std::shared_ptr<Window>> WindowList;

//
// Time state
//

// Public time info, hidden behind const.
struct Time {
    double current;
    double dt;
    double framerate;
    size_t frameIndex;
};

//
// Input state
//

struct PressState {
    unsigned pressCount;      // number of consecutive times button has been pressed (within x milliseconds)
    double   elapsedTime;     // time since last press / release, in seconds

    bool pressed () { return pressCount != 0; }
};

// Double buffered mouse state (readonly).
struct Mouse {
    struct State {
        typedef std::array<PressState, GLFW_MOUSE_BUTTON_LAST+1> ButtonArray;

        glm::vec2   pos;
        glm::vec2   scroll;
        ButtonArray buttons;
    } state[2];

    // Get button state (convenience method)
    const PressState& get (int button, unsigned state = 0) const {
        return state[std::min(1, state)].buttons[button];
    }
};

// Double buffered keyboard state (readonly).
struct Keyboard {
    struct State {
        typedef std::array<PressState, GLFW_KEY_LAST+1> KeyArray;

        KeyArray keys;
    } state[2];

    // Get key state (convenience method)
    const PressState& get (int key, unsigned state = 0) const {
        return state[std::min(1, state)].keys[key];
    }
};

//
// Input state (gamepad)
//

enum class GamepadButton {
    X, Y, A, B, LB, RB, LT, RT, LS, RS,
    DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT,
    START, BACK, HOME,
};
constexpr size_t NUM_GAMEPAD_BUTTONS = GamepadButton::HOME;

enum class GamepadAxis {
    LX, LY, RX, RY, LT, RT, 
    DPAD_X, DPAD_Y, 
};
constexpr size_t NUM_GAMEPAD_AXES = GamepadAxis::DPAD_Y;

enum class GamepadFlags {
    IS_XBOX_LIKE = 1 << 1,
    IS_PS_LIKE   = 1 << 2,
};
struct Gamepad {
    unsigned        id;
    GamepadFlags    flags;

    struct State {
        typedef std::array<PressState, NUM_GAMEPAD_BUTTONS> ButtonArray;
        typedef std::array<double,     NUM_GAMEPAD_AXES>    AxisArray;

        ButtonArray buttons;
        AxisArray   axes;       // always normalized to [-1, 1] (X/Y), or [0, 1] (triggers)
    } state[2];

    // Get button state (convenience method)
    const PressState& get (GamepadButton btn, unsigned state = 0) const {
        return state[std::min(1, state)].buttons[btn];
    }
    // Get axis state (convenience method)
    double get (GamepadAxis axis, unsigned state = 0) const {
        return state[std::min(1, state)].axes[axis];
    }
};
typedef std::vector<Gamepad> GamepadList;

// See gamepad_mapper.hpp
class DeviceManager {
    template <class GamepadMapper>
    void registerGamepadMapper ();

    template <class GamepadMapper>
    void unregisterGamepadMapper ();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
public:
    DeviceManager  (Impl* impl);
    ~DeviceManager ();

    // non-copyable
    DeviceManager (const DeviceManager&) = delete;
    DeviceManager& operator= (const DeviceManager&) = delete;
};

//
// Public application state
//

class ApplicationState {
    WindowList          windows;
    const ScreenList*   screens;

    const Time*         time;
    const Mouse*        mouse;
    const Keyboard*     keyboard;
    const GamepadList*  gamepads;

    DeviceManager       deviceManager;
};
