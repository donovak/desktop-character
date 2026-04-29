#include "AppConfig.h"

#include <vector>

namespace {
std::vector<std::wstring> tokenizeCommandLine(std::wstring_view commandLine)
{
    std::vector<std::wstring> tokens;
    std::wstring current;
    bool inQuotes = false;

    for (wchar_t character : commandLine) {
        if (character == L'"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (!inQuotes && (character == L' ' || character == L'\t')) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(character);
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}
}

AppConfig AppConfig::fromCommandLine(std::wstring_view commandLine)
{
    AppConfig config {};
    const std::vector<std::wstring> tokens = tokenizeCommandLine(commandLine);

    if (commandLine.find(L"--normal-window") != std::wstring_view::npos) {
        config.windowMode = WindowMode::NormalWindow;
    }

    if (commandLine.find(L"--desktop-overlay") != std::wstring_view::npos) {
        config.windowMode = WindowMode::DesktopOverlay;
    }

    if (commandLine.find(L"--opaque-overlay") != std::wstring_view::npos) {
        config.transparentOverlayBackground = false;
    }

    if (commandLine.find(L"--enable-click-through") != std::wstring_view::npos) {
        config.enableClickThrough = true;
    }

    if (commandLine.find(L"--dry-run-interactions") != std::wstring_view::npos) {
        config.dryRunInteractions = true;
    }

    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (tokens[index] == L"--log-file" && index + 1 < tokens.size()) {
            config.logFilePath = tokens[index + 1];
            break;
        }

        constexpr std::wstring_view LOG_FILE_PREFIX = L"--log-file=";
        if (tokens[index].starts_with(LOG_FILE_PREFIX)) {
            config.logFilePath = tokens[index].substr(LOG_FILE_PREFIX.size());
            break;
        }
    }

    return config;
}
