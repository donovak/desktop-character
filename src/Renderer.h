#pragma once

#include "Character.h"

#include <d2d1.h>
#include <wrl/client.h>
#include <windows.h>

class Renderer {
public:
    Renderer() = default;

    bool initialize(HWND hwnd);
    void resizeIfNeeded(unsigned int width, unsigned int height);
    void render(const Character& character);

private:
    bool createDeviceResources(HWND hwnd);
    void discardDeviceResources();

    Microsoft::WRL::ComPtr<ID2D1Factory> m_factory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_characterBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_backgroundBrush;
    HWND m_hwnd = nullptr;
};
