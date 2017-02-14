#pragma once

#include "types.hxx"
#include "property.hxx"

namespace k {
namespace app {

struct Screen {
    int         monitor;
    glm::ivec2  dimensions;
    bool        fullscreen;
};

class WindowImpl;
class WindowManagerImpl;

// Buffered, threadsafe window handle, providing
// access to window properties.
class Window {
    std::unique_ptr<WindowImpl> impl;
public:
    // Constructs a backing window reference from this window
    void create ();

    // Tears down this window + removes the current window reference from WindowManager
    void destroy ();

    // Set window active / inactive.
    Property<bool> active;

    // Get / Set Window Name (changes reference via WindowManager!)
    Property<std::string> name;

    // Get / Set Window title
    Property<std::string> title;

    // Get / Set Window Size
    Property<glm::ivec2>  size;

    // Get / Set Window Screen
    Property<const Screen&> screen;

    enum class PixelScaleFactor { 
        AUTOMATIC = 0,  // automatic / default
        FIXED_1X  = 1,  // fixed "standard res"
        FIXED_2X  = 2,  // fixed "retina / quad res"
    };

    // Get / Set ScaleFactor (set 0 => automatic)
    Property<PixelScaleFactor>  pixelScaleFactor;

    // Create window (call this once)
    void create ();
    void destroy ();

    Window (void*);
    virtual ~Window ();
};

// Window manager object owned by AppInstance.
class WindowManager {
    std::unique_ptr<WindowManagerImpl> impl;
public:
    // Create / get window with the specified name.
    Window& operator[] (const std::string& name);

    typedef const std::vector<std::string>& WindowList;
    typedef const std::vector<Screen>& ScreenList;

    // Get list of all windows (by name)
    const std::vector<std::string>& windows ();

    // Get list of all screens (by name)
    const std::vector<const Screen*>& screens ();

    static const Screen* largestFullscreen (ScreenList);
    static const Screen* largestWindowed   (ScreenList);

    WindowManager (void*);
};

}; // namespace app
}; // namespace k
