#include "DesktopWindow.h"

#include "DebugLog.h"

#include <string>

namespace {
constexpr wchar_t WINDOW_CLASS_NAME[] = L"DesktopCharacterWindowClass";
constexpr wchar_t WINDOW_TITLE[] = L"Desktop Character Prototype";
constexpr int DEFAULT_WIDTH = 960;
constexpr int DEFAULT_HEIGHT = 540;

COLORREF overlayTransparentColorKey()
{
    return RGB(
        OVERLAY_TRANSPARENT_KEY_R,
        OVERLAY_TRANSPARENT_KEY_G,
        OVERLAY_TRANSPARENT_KEY_B);
}

RECT virtualDesktopRect()
{
    const int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return { x, y, x + width, y + height };
}

std::wstring rectToString(const RECT& rect)
{
    return L"left=" + std::to_wstring(rect.left)
        + L", top=" + std::to_wstring(rect.top)
        + L", width=" + std::to_wstring(rect.right - rect.left)
        + L", height=" + std::to_wstring(rect.bottom - rect.top);
}
}

DesktopWindow::~DesktopWindow()
{
    if (m_hwnd != nullptr) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool DesktopWindow::create(HINSTANCE instance, int showCommand, AppConfig config)
{
    m_config = config;

    WNDCLASSEXW windowClass {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DesktopWindow::windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS_NAME;

    if (RegisterClassExW(&windowClass) == 0) {
        debugLog(L"RegisterClassExW failed.");
        return false;
    }

    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    DWORD extendedStyle = 0;
    RECT windowRect { 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT };
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;

    if (m_config.windowMode == WindowMode::DesktopOverlay) {
        windowStyle = WS_POPUP;
        extendedStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;

        if (m_config.transparentOverlayBackground) {
            extendedStyle |= WS_EX_LAYERED;
        }

        if (m_config.enableClickThrough) {
            extendedStyle |= WS_EX_TRANSPARENT;
        }

        windowRect = virtualDesktopRect();
        x = windowRect.left;
        y = windowRect.top;
        debugLog(std::wstring(L"Creating DesktopOverlay window: ") + rectToString(windowRect));
    } else {
        if (!AdjustWindowRectEx(&windowRect, windowStyle, FALSE, extendedStyle)) {
            debugLog(L"AdjustWindowRectEx failed.");
            return false;
        }

        debugLog(L"Creating NormalWindow.");
    }

    m_hwnd = CreateWindowExW(
        extendedStyle,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        windowStyle,
        x,
        y,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        this);

    if (m_hwnd == nullptr) {
        debugLog(L"CreateWindowExW failed.");
        return false;
    }

    if (m_config.windowMode == WindowMode::DesktopOverlay && m_config.transparentOverlayBackground) {
        if (!SetLayeredWindowAttributes(m_hwnd, overlayTransparentColorKey(), 255, LWA_COLORKEY)) {
            debugLog(L"SetLayeredWindowAttributes failed; overlay may appear opaque.");
        }
    }

    m_running = true;
    ShowWindow(m_hwnd, m_config.windowMode == WindowMode::DesktopOverlay ? SW_SHOWNOACTIVATE : showCommand);
    UpdateWindow(m_hwnd);
    return true;
}

HWND DesktopWindow::handle() const
{
    return m_hwnd;
}

RECT DesktopWindow::clientRect() const
{
    RECT rect {};
    if (m_hwnd != nullptr) {
        GetClientRect(m_hwnd, &rect);
    }

    return rect;
}

bool DesktopWindow::isRunning() const
{
    return m_running;
}

void DesktopWindow::requestClose()
{
    m_running = false;
    if (m_hwnd != nullptr) {
        PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
    }
}

LRESULT CALLBACK DesktopWindow::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DesktopWindow* window = nullptr;

    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<DesktopWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    } else {
        window = reinterpret_cast<DesktopWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window != nullptr) {
        return window->handleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT DesktopWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_DISPLAYCHANGE:
        debugLog(L"WM_DISPLAYCHANGE received.");
        if (m_config.windowMode == WindowMode::DesktopOverlay) {
            applyOverlayBounds();
        }
        return 0;
    case WM_DPICHANGED:
        debugLog(L"WM_DPICHANGED received.");
        if (m_config.windowMode == WindowMode::DesktopOverlay) {
            applyOverlayBounds();
            return 0;
        }

        if (lParam != 0) {
            const auto* suggestedRect = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(
                m_hwnd,
                nullptr,
                suggestedRect->left,
                suggestedRect->top,
                suggestedRect->right - suggestedRect->left,
                suggestedRect->bottom - suggestedRect->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        break;
    case WM_MOUSEACTIVATE:
        if (m_config.windowMode == WindowMode::DesktopOverlay) {
            return MA_NOACTIVATE;
        }
        return DefWindowProcW(m_hwnd, message, wParam, lParam);
    case WM_CLOSE:
        m_running = false;
        DestroyWindow(m_hwnd);
        return 0;
    case WM_DESTROY:
        m_hwnd = nullptr;
        requestClose();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(m_hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

void DesktopWindow::applyOverlayBounds()
{
    if (m_hwnd == nullptr) {
        return;
    }

    const RECT rect = virtualDesktopRect();
    debugLog(std::wstring(L"Applying DesktopOverlay bounds: ") + rectToString(rect));

    SetWindowPos(
        m_hwnd,
        nullptr,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}
