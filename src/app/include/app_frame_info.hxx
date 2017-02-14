
#pragma once

namespace k {
namespace app {

struct FrameInfo {
    struct {
        double dt;          // delta-time since last frame
        double localTime;   // local time since app startup
    } time;
};

}; // namespace app
}; // namespace k
