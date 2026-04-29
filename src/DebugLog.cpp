#include "DebugLog.h"

#include <cstdio>
#include <fstream>
#include <filesystem>
#include <string>
#include <windows.h>

namespace {
std::wofstream& logFile()
{
    static std::wofstream stream;
    return stream;
}
}

void configureDebugLogFile(std::wstring_view path)
{
    if (path.empty()) {
        return;
    }

    logFile().open(std::filesystem::path(path), std::ios::out | std::ios::app);
    if (logFile().is_open()) {
        debugLog(std::wstring(L"File logging enabled: ") + std::wstring(path));
    } else {
        debugLog(std::wstring(L"Failed to open log file: ") + std::wstring(path));
    }
}

void debugLog(std::wstring_view message)
{
    std::wstring line(message);
    line.append(L"\n");
    OutputDebugStringW(line.c_str());

    if (GetConsoleWindow() != nullptr) {
        std::fwprintf(stderr, L"%ls", line.c_str());
        std::fflush(stderr);
    }

    if (logFile().is_open()) {
        logFile() << line;
        logFile().flush();
    }
}
