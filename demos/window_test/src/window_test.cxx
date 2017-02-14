
#include <iostream>
#include <app.hxx>
using namespace k::app;

class WindowTest : public AppClient {
    void onAppInit (AppInstance& app) {
        std::cout << "Init\n";
    }
    void onAppTeardown (AppInstance& app) {
        std::cout << "Teardown\n";
    }
};

int main (int argc, const char** argv) {
    AppLauncher launcher;
    AppConfig config;

    launcher.registerClient<WindowTest>();
    return launcher.launch(config, argc, argv);
}
