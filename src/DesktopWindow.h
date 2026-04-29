#pragma once

#include "AppConfig.h"

#include <windows.h>

class DesktopWindow {
public:
    DesktopWindow() = default;
    ~DesktopWindow();

    DesktopWindow(const DesktopWindow&) = delete;
    DesktopWindow& operator=(const DesktopWindow&) = delete;

    bool create(HINSTANCE instance, int showCommand, AppConfig config);
    HWND handle() const;
    RECT clientRect() const;
    POINT clientScreenOrigin() const;
    bool isRunning() const;
    void requestClose();

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void applyOverlayBounds();

    AppConfig m_config;
    HWND m_hwnd = nullptr;
    bool m_running = false;
};
