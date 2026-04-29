#pragma once

#include <string>
#include <vector>
#include <windows.h>

struct DesktopIcon {
    std::wstring displayName;
    RECT screenBounds {};
    RECT imageBounds {};
    RECT labelBounds {};
    POINT anchorPoint {};
    int labelLineCount = 1;
    std::wstring debugIdentifier;
    std::wstring filesystemPath;
};

class DesktopIconService {
public:
    DesktopIconService();
    ~DesktopIconService() = default;

    DesktopIconService(const DesktopIconService&) = delete;
    DesktopIconService& operator=(const DesktopIconService&) = delete;

    std::vector<DesktopIcon> refresh();

private:
    class ComApartment {
    public:
        ComApartment();
        ~ComApartment();

        ComApartment(const ComApartment&) = delete;
        ComApartment& operator=(const ComApartment&) = delete;

        bool isInitialized() const;

    private:
        HRESULT m_result = E_FAIL;
        bool m_shouldUninitialize = false;
    };

    ComApartment m_comApartment;
};
