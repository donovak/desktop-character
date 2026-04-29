#include "DesktopWindow.h"

namespace {
constexpr wchar_t WINDOW_CLASS_NAME[] = L"DesktopCharacterWindowClass";
constexpr wchar_t WINDOW_TITLE[] = L"Desktop Character Prototype";
constexpr int DEFAULT_WIDTH = 960;
constexpr int DEFAULT_HEIGHT = 540;
}

DesktopWindow::~DesktopWindow()
{
    if (m_hwnd != nullptr) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool DesktopWindow::create(HINSTANCE instance, int showCommand)
{
    WNDCLASSEXW windowClass {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DesktopWindow::windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS_NAME;

    if (RegisterClassExW(&windowClass) == 0) {
        return false;
    }

    RECT windowRect { 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT };
    if (!AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0)) {
        return false;
    }

    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        this);

    if (m_hwnd == nullptr) {
        return false;
    }

    m_running = true;
    ShowWindow(m_hwnd, showCommand);
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
    case WM_CLOSE:
        requestClose();
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
}
