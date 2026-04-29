#include "Renderer.h"

#include "AppConfig.h"
#include "DebugLog.h"

namespace {
D2D1_COLOR_F overlayTransparentColor()
{
    return D2D1::ColorF(
        OVERLAY_TRANSPARENT_KEY_R / 255.0f,
        OVERLAY_TRANSPARENT_KEY_G / 255.0f,
        OVERLAY_TRANSPARENT_KEY_B / 255.0f);
}
}

bool Renderer::initialize(HWND hwnd, bool transparentBackground)
{
    m_hwnd = hwnd;
    m_transparentBackground = transparentBackground;

    const HRESULT factoryResult = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        m_factory.GetAddressOf());

    if (FAILED(factoryResult)) {
        debugLog(L"D2D1CreateFactory failed.");
        return false;
    }

    const HRESULT writeFactoryResult = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf()));

    if (FAILED(writeFactoryResult)) {
        debugLog(L"DWriteCreateFactory failed.");
        return false;
    }

    const HRESULT textFormatResult = m_writeFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"",
        m_iconTextFormat.GetAddressOf());

    if (FAILED(textFormatResult)) {
        debugLog(L"CreateTextFormat failed.");
        return false;
    }

    return createDeviceResources(hwnd);
}

void Renderer::resizeIfNeeded(unsigned int width, unsigned int height)
{
    if (m_renderTarget == nullptr || width == 0 || height == 0) {
        return;
    }

    const D2D1_SIZE_U currentSize = m_renderTarget->GetPixelSize();
    if (currentSize.width == width && currentSize.height == height) {
        return;
    }

    const HRESULT result = m_renderTarget->Resize(D2D1::SizeU(width, height));
    if (FAILED(result)) {
        discardDeviceResources();
        createDeviceResources(m_hwnd);
    }
}

void Renderer::render(
    const Character& character,
    const std::vector<DesktopIcon>& desktopIcons,
    bool showIconDebugOverlay,
    POINT clientScreenOrigin)
{
    if (m_renderTarget == nullptr && !createDeviceResources(m_hwnd)) {
        return;
    }

    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(
        m_transparentBackground ? overlayTransparentColor() : D2D1::ColorF(0.08f, 0.09f, 0.10f));

    const D2D1_RECT_F characterRect = character.bounds();
    m_renderTarget->FillRectangle(characterRect, m_characterBrush.Get());

    if (showIconDebugOverlay) {
        drawIconDebugOverlay(desktopIcons, clientScreenOrigin);
    }

    const HRESULT result = m_renderTarget->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
        discardDeviceResources();
    } else if (FAILED(result)) {
        debugLog(L"Direct2D EndDraw failed.");
        discardDeviceResources();
    }
}

bool Renderer::createDeviceResources(HWND hwnd)
{
    if (m_factory == nullptr || hwnd == nullptr) {
        return false;
    }

    RECT clientRect {};
    GetClientRect(hwnd, &clientRect);

    const D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<unsigned int>(clientRect.right - clientRect.left),
        static_cast<unsigned int>(clientRect.bottom - clientRect.top));

    if (size.width == 0 || size.height == 0) {
        return false;
    }

    HRESULT result = m_factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        m_renderTarget.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateHwndRenderTarget failed.");
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.14f, 0.73f, 0.48f),
        m_characterBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed.");
        discardDeviceResources();
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.95f, 0.80f, 0.24f, 0.95f),
        m_iconBoundsBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for icon bounds.");
        discardDeviceResources();
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.95f, 0.95f, 0.95f, 0.95f),
        m_iconTextBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for icon text.");
        discardDeviceResources();
        return false;
    }

    return true;
}

void Renderer::discardDeviceResources()
{
    m_iconTextBrush.Reset();
    m_iconBoundsBrush.Reset();
    m_characterBrush.Reset();
    m_backgroundBrush.Reset();
    m_renderTarget.Reset();
}

void Renderer::drawIconDebugOverlay(const std::vector<DesktopIcon>& desktopIcons, POINT clientScreenOrigin)
{
    if (m_iconBoundsBrush == nullptr || m_iconTextBrush == nullptr || m_iconTextFormat == nullptr) {
        return;
    }

    for (const DesktopIcon& icon : desktopIcons) {
        const D2D1_RECT_F iconRect = D2D1::RectF(
            static_cast<float>(icon.screenBounds.left - clientScreenOrigin.x),
            static_cast<float>(icon.screenBounds.top - clientScreenOrigin.y),
            static_cast<float>(icon.screenBounds.right - clientScreenOrigin.x),
            static_cast<float>(icon.screenBounds.bottom - clientScreenOrigin.y));

        m_renderTarget->DrawRectangle(iconRect, m_iconBoundsBrush.Get(), 1.5f);

        if (!icon.displayName.empty()) {
            const D2D1_RECT_F textRect = D2D1::RectF(
                iconRect.left,
                iconRect.bottom + 2.0f,
                iconRect.left + 220.0f,
                iconRect.bottom + 24.0f);

            m_renderTarget->DrawTextW(
                icon.displayName.c_str(),
                static_cast<UINT32>(icon.displayName.size()),
                m_iconTextFormat.Get(),
                textRect,
                m_iconTextBrush.Get());
        }
    }
}
