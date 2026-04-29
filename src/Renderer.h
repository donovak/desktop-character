#pragma once

#include "Character.h"
#include "DesktopIconService.h"

#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <wrl/client.h>
#include <windows.h>

struct IconDebugOverlaySettings {
    bool showOverlay = true;
    bool showHoverBounds = true;
    bool showImageBounds = true;
    bool showAnchors = true;
    bool showLabels = true;
};

class Renderer {
public:
    Renderer() = default;

    bool initialize(HWND hwnd, bool transparentBackground);
    void resizeIfNeeded(unsigned int width, unsigned int height);
    void render(
        const Character& character,
        const std::vector<DesktopIcon>& desktopIcons,
        const IconDebugOverlaySettings& iconDebugOverlaySettings,
        int interactableIconIndex,
        POINT clientScreenOrigin);

private:
    bool createDeviceResources(HWND hwnd);
    void discardDeviceResources();
    void drawIconDebugOverlay(
        const std::vector<DesktopIcon>& desktopIcons,
        const IconDebugOverlaySettings& settings,
        int interactableIconIndex,
        POINT clientScreenOrigin);

    Microsoft::WRL::ComPtr<ID2D1Factory> m_factory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_characterBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_backgroundBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_iconHoverBoundsBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_iconImageBoundsBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_iconAnchorBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_iconTextBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_interactableIconBrush;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_writeFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_iconTextFormat;
    HWND m_hwnd = nullptr;
    bool m_transparentBackground = false;
};
