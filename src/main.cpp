#include "App.h"
#include "AppConfig.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCommand)
{
    const AppConfig config = AppConfig::fromCommandLine(
        commandLine != nullptr ? std::wstring_view(commandLine) : std::wstring_view {});

    App app(instance, showCommand, config);
    return app.run();
}
