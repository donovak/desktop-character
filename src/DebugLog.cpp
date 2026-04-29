#include "DebugLog.h"

#include <string>
#include <windows.h>

void debugLog(std::wstring_view message)
{
    std::wstring line(message);
    line.append(L"\n");
    OutputDebugStringW(line.c_str());
}
