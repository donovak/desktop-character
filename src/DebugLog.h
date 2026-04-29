#pragma once

#include <string_view>

void configureDebugLogFile(std::wstring_view path);
void debugLog(std::wstring_view message);
