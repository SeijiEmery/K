
#pragma once

//
// Gamepad mapping
//

// Gamepad configuration info. Set by application user via UI + stored persistently; 
// appears only as readonly data for gamepad mappers + user code.
//
// Note: each gamepad type has its own gamepad config (so we can tweak deadzones, etc),
// though we do also have a default config that applies to all gamepads.
//
struct GamepadConfig {
    enum class Flags {
        FLIP_LY, FLIP_LX, 
        FLIP_RY, FLIP_RX,
        Count
    };
    // User settings for eg. flip axis LY up / down.
    std::bitset<Flags::Count>           flags;

    // Deadzones for each axis. (note: includes axes that don't really have deadzones...)
    std::array<float, NUM_GAMEPAD_AXES> deadzones;

    // List of thresholds, in seconds, allowed between subsequent button presses (otherwise times out + resets press count).
    // length determines the max number of allowed presses.
    // Implemented internally.
    std::vector<double>                 pressCountThreshold;
};

// Implement this for each support gamepad type.
class IGamepadMapper {
    typedef Gamepad::State::AxisArray   AxisArray;
    typedef Gamepad::State::ButtonArray ButtonArray;

    // Return a unique string identifying this gamepad type (recommended: "XBOX360", "DS4", etc)
    virtual const std::string& gamepadType () const = 0;

    // Return any gamepad flags associated w/ this gamepad type (for now, just either IS_XBOX_LIKE or IS_PS_LIKE)
    virtual GamepadFlags gamepadFlags () const = 0;

    // Return true iff our gamepad profile matches this type (most gamepads have a unique # of buttons / axes).
    // Note: the vendorString is OS and driver dependent, so this should NOT be matching against this. But since
    // it might be useful for debugging, we're keeping it anyways.
    virtual bool matches  (size_t numButtons, size_t numAxes, const char* vendorString) = 0;

    virtual void mapInput (
        const int*      buttonInputs, 
        const float*    axisInputs,
        const GamepadConfig& config,

        ButtonArray&    buttonOutputs, 
        AxisArray&      axisOutputs
    ) = 0;
};

template <typename Mapping>
class GamepadMapper : public IGamepadMapper {
    const std::string& gamepadType  () const override { return Mapping::name; }
    GamepadFlags       gamepadFlags () const override { return Mapping::flags; }

    // Default implementation; can further override
    bool matches (size_t numButtons, size_t numAxes, const char* vendorString) override {
        return numButtons == Mapping::buttons.size() && numAxes == Mapping::axes.size();
    }

    // Default implementation; can further override
    void mapInput (
        const int*      buttonInputs, 
        const float*    axisInputs,
        const GamepadConfig& config,

        ButtonArray&    buttonOutputs, 
        AxisArray&      axisOutputs
    ) override {
        mapButtons(buttonInputs, buttonOutputs, Mapping::buttons);
        mapAxis(axisInputs, axisOutputs, Mapping::axes, Mapping::normalizeTriggers);
        if (Mapping::createTriggerButtons) mapTriggersToButtons(buttons, axes, triggerThreshold);
        if (Mapping::createDpadAxes)       mapDpadButtonsToAxes(buttons, axes);
    }

    //
    // Helper methods (so you don't have to write your own implementation for this stuff)
    //

    // Call this to correctly set button pressed / unpressed.
    // Note: pressTime, etc., is handled internally.
    static void setPressed (PressState& button, bool pressed) {
        if (pressed) ++button.pressCount;
        else         button.pressCount = 0;
    }
    static void setAxis (double& output, float rawValue, float deadzone) {
        output = abs(rawValue) >= deadzone ? (double)rawValue : 0.0;
    }
    static float axisFromButtons (const PressState& plus, const PressState& minus) {
        return (float)std::min(plus.pressCount, 1)) - (float)(std::min(minus.pressCount, 1));
    }

    template <size_t N>
    static void mapAxes (
        const float* axisValues, AxisArray& out, const GamepadConfig& config,
        const std::array<GamepadAxis, N>& axes,  // Your mapped axes
        bool normalizeTriggers                   // set true to normalize directx triggers from [-1,1] to [0,1], or false if raw inputs already [0,1]
    ) {
        size_t i = 0;
        for (auto axisIndex : axes) {
            setAxis(out[axisIndex], axisValues[i++], config.deadzones[axisIndex]);
        }
        if (normalizeTriggers) {
            out[GamepadAxis::LTRIGGER] = 0.5 * (out[GamepadAxis::LTRIGGER] + 1.0);
            out[GamepadAxis::RTRIGGER] = 0.5 * (out[GamepadAxis::RTRIGGER] + 1.0);
        }
    }
    template <size_t N>
    static void mapButtons (
        const int* buttonValues, Gamepad::State::ButtonArray& out, const GamepadConfig& config,
        const std::array<GamepadButton, N>& buttons,
    ) {
        size_t i = 0;
        for (auto buttonIndex : buttons) {
            setPressed(out[buttonIndex], buttonValues[i++] != 0);
        }
    }
    static void mapTriggersToButtons (ButtonArray& buttons, AxisArray& axes, double threshold = 0.5) {
        setPressed(buttons[GamepadButton::LTRIGGER], axes[GamepadButton::LTRIGGER] != 0.0);
        setPressed(buttons[GamepadButton::RTRIGGER], axes[GamepadButton::RTRIGGER] != 0.0);
    }
    static void mapDpadButtonsToAxes (ButtonArray& buttons, AxisArray& axes) {
        axes[GamepadAxis::DPAD_X] = buttonsToAxis(buttons[GamepadButton::DPAD_LEFT], buttons[GamepadButton::DPAD_RIGHT]);
        axes[GamepadAxis::DPAD_Y] = buttonsToAxis(buttons[GamepadButton::DPAD_UP],   buttons[GamepadButton::DPAD_DOWN]);
    }
};

//
// Usage:
//

struct DS4Mapping {
    static constexpr std::string  name = "DS4";
    static constexpr GamepadFlags flags = GamepadFlags::IS_PS_LIKE;
    static constexpr bool normalizeTriggers = false;
    static constexpr bool createTriggerButtons = false;
    static constexpr bool createDpadAxes = true;
    static std::array<GamepadButton, 18> buttons {
        GamepadButton::X,  // square
        GamepadButton::A,  // x
        GamepadButton::B,  // circle
        GamepadButton::Y,  // triangle
        GamepadButton::LBUMPER,
        GamepadButton::RBUMPER,
        GamepadButton::LTRIGGER, // ds4 actually has triggers aliased as buttons, apparently
        GamepadButton::RTRIGGER,
        GamepadButton::SELECT, // share button
        GamepadButton::START,
        GamepadButton::LSTICK,
        GamepadButton::RSTICK,
        GamepadButton::HOME,
        GamepadButton::SELECT, // center button
        GamepadButton::DPAD_UP,
        GamepadButton::DPAD_RIGHT,
        GamepadButton::DPAD_DOWN,
        GamepadButton::DPAD_LEFT,
    }
    static std::array<GamepadAxis, 6> axes {
        GamepadAxis::LX,
        GamepadAxis::LY,
        GamepadAxis::RX,
        GamepadAxis::RY,
        GamepadAxis::LTRIGGER,
        GamepadAxis::RTRIGGER
    };
};
class DS4Mapper : public GamepadMapper<DS4Mapping> {};
