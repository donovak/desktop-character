#include "Renderer.h"

#include "AppConfig.h"
#include "DebugLog.h"

#include <string>

namespace {
constexpr float DEBUG_HOVER_STROKE_WIDTH = 1.5f;
constexpr float DEBUG_IMAGE_STROKE_WIDTH = 1.0f;
constexpr float DEBUG_INTERACTABLE_STROKE_WIDTH = 3.0f;
constexpr float DEBUG_BUMPED_STROKE_WIDTH = 3.0f;
constexpr float DEBUG_ANCHOR_MARK_SIZE = 4.0f;
constexpr float DEBUG_TEXT_TOP_GAP = 1.0f;
constexpr float DASH_VISUAL_STROKE_WIDTH = 5.0f;
constexpr wchar_t CHARACTER_SPRITE_PATH[] = L"assets\\sprites\\hamster_sheet.png";
constexpr wchar_t CHARACTER_SPRITE_BUILD_PATH[] = L"..\\..\\assets\\sprites\\hamster_sheet.png";

D2D1_COLOR_F overlayTransparentColor()
{
    return D2D1::ColorF(
        OVERLAY_TRANSPARENT_KEY_R / 255.0f,
        OVERLAY_TRANSPARENT_KEY_G / 255.0f,
        OVERLAY_TRANSPARENT_KEY_B / 255.0f);
}

std::wstring findCharacterSpritePath()
{
    if (GetFileAttributesW(CHARACTER_SPRITE_PATH) != INVALID_FILE_ATTRIBUTES) {
        return CHARACTER_SPRITE_PATH;
    }

    if (GetFileAttributesW(CHARACTER_SPRITE_BUILD_PATH) != INVALID_FILE_ATTRIBUTES) {
        return CHARACTER_SPRITE_BUILD_PATH;
    }

    return CHARACTER_SPRITE_PATH;
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

    const HRESULT wicFactoryResult = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(m_wicFactory.GetAddressOf()));

    if (FAILED(wicFactoryResult)) {
        debugLog(L"CoCreateInstance failed for WIC imaging factory; character will use rectangle fallback.");
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

    m_iconTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_iconTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    m_iconTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

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
    const IconDebugOverlaySettings& iconDebugOverlaySettings,
    int interactableIconIndex,
    int bumpedIconIndex,
    bool controlModeEnabled,
    POINT clientScreenOrigin)
{
    if (m_renderTarget == nullptr && !createDeviceResources(m_hwnd)) {
        return;
    }

    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(
        m_transparentBackground ? overlayTransparentColor() : D2D1::ColorF(0.08f, 0.09f, 0.10f));

    drawDashVisual(character);
    drawCharacter(character);

    if (iconDebugOverlaySettings.showOverlay) {
        drawIconDebugOverlay(desktopIcons, iconDebugOverlaySettings, interactableIconIndex, bumpedIconIndex, clientScreenOrigin);
    }

    if (controlModeEnabled) {
        drawControlModeIndicator();
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
        m_iconHoverBoundsBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for icon hover bounds.");
        discardDeviceResources();
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.15f, 0.72f, 1.0f, 0.95f),
        m_iconImageBoundsBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for icon image bounds.");
        discardDeviceResources();
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 0.2f, 0.2f, 0.95f),
        m_iconAnchorBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for icon anchors.");
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

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.25f, 1.0f, 0.35f, 1.0f),
        m_interactableIconBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for interactable icon highlight.");
        discardDeviceResources();
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 0.25f, 0.95f, 1.0f),
        m_bumpedIconBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for bumped icon highlight.");
        discardDeviceResources();
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.15f, 0.95f, 1.0f, 0.75f),
        m_dashVisualBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for dash visual.");
        discardDeviceResources();
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.12f, 1.0f, 0.55f, 0.95f),
        m_controlModeBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"CreateSolidColorBrush failed for control mode indicator.");
        discardDeviceResources();
        return false;
    }

    loadCharacterSpriteSheet();
    return true;
}

bool Renderer::loadCharacterSpriteSheet()
{
    if (m_wicFactory == nullptr || m_renderTarget == nullptr) {
        return false;
    }

    const std::wstring spritePath = findCharacterSpritePath();

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT result = m_wicFactory->CreateDecoderFromFilename(
        spritePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        decoder.GetAddressOf());

    if (FAILED(result)) {
        debugLog(std::wstring(L"Failed to load character sprite sheet: ") + spritePath);
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    result = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(result)) {
        debugLog(L"Failed to read first frame from character sprite sheet.");
        return false;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    result = m_wicFactory->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(result)) {
        debugLog(L"Failed to create WIC format converter for character sprite sheet.");
        return false;
    }

    result = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeMedianCut);

    if (FAILED(result)) {
        debugLog(L"Failed to convert character sprite sheet to PBGRA.");
        return false;
    }

    result = m_renderTarget->CreateBitmapFromWicBitmap(
        converter.Get(),
        nullptr,
        m_characterSpriteSheet.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        debugLog(L"Failed to create Direct2D bitmap for character sprite sheet.");
        return false;
    }

    debugLog(std::wstring(L"Loaded character sprite sheet: ") + spritePath);
    return true;
}

void Renderer::drawCharacter(const Character& character)
{
    if (m_characterSpriteSheet == nullptr) {
        m_renderTarget->FillRectangle(character.bounds(), m_characterBrush.Get());
        return;
    }

    const D2D1_RECT_F destination = character.spriteDestinationBounds();
    const D2D1_RECT_F source = character.spriteSourceRect();

    D2D1_MATRIX_3X2_F oldTransform {};
    m_renderTarget->GetTransform(&oldTransform);

    const float centerX = (destination.left + destination.right) * 0.5f;
    const float centerY = (destination.top + destination.bottom) * 0.5f;
    D2D1_MATRIX_3X2_F spriteTransform = D2D1::Matrix3x2F::Identity();

    if (!character.isFacingRight()) {
        spriteTransform = D2D1::Matrix3x2F::Scale(
            D2D1::SizeF(-1.0f, 1.0f),
            D2D1::Point2F(centerX, centerY))
            * spriteTransform;
    }

    m_renderTarget->SetTransform(spriteTransform * oldTransform);

    m_renderTarget->DrawBitmap(
        m_characterSpriteSheet.Get(),
        destination,
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        source);

    m_renderTarget->SetTransform(oldTransform);
}

void Renderer::drawDashVisual(const Character& character)
{
    if (!character.isDashVisualActive() || m_dashVisualBrush == nullptr) {
        return;
    }

    const Vector2 start = character.dashStartPosition();
    const Vector2 end = character.dashEndPosition();
    m_renderTarget->DrawLine(
        D2D1::Point2F(start.x, start.y),
        D2D1::Point2F(end.x, end.y),
        m_dashVisualBrush.Get(),
        DASH_VISUAL_STROKE_WIDTH);

    const D2D1_ELLIPSE endFlash = D2D1::Ellipse(D2D1::Point2F(end.x, end.y), 16.0f, 16.0f);
    m_renderTarget->DrawEllipse(endFlash, m_dashVisualBrush.Get(), 2.0f);
}

void Renderer::drawControlModeIndicator()
{
    if (m_controlModeBrush == nullptr || m_iconTextFormat == nullptr) {
        return;
    }

    const D2D1_RECT_F indicatorRect = D2D1::RectF(10.0f, 10.0f, 150.0f, 30.0f);
    m_renderTarget->DrawRectangle(indicatorRect, m_controlModeBrush.Get(), 1.5f);
    m_renderTarget->DrawTextW(
        L"CONTROL MODE",
        12,
        m_iconTextFormat.Get(),
        indicatorRect,
        m_controlModeBrush.Get());
}

void Renderer::discardDeviceResources()
{
    m_controlModeBrush.Reset();
    m_dashVisualBrush.Reset();
    m_characterSpriteSheet.Reset();
    m_bumpedIconBrush.Reset();
    m_interactableIconBrush.Reset();
    m_iconTextBrush.Reset();
    m_iconAnchorBrush.Reset();
    m_iconImageBoundsBrush.Reset();
    m_iconHoverBoundsBrush.Reset();
    m_characterBrush.Reset();
    m_backgroundBrush.Reset();
    m_renderTarget.Reset();
}

void Renderer::drawIconDebugOverlay(
    const std::vector<DesktopIcon>& desktopIcons,
    const IconDebugOverlaySettings& settings,
    int interactableIconIndex,
    int bumpedIconIndex,
    POINT clientScreenOrigin)
{
    if (m_iconHoverBoundsBrush == nullptr
        || m_iconImageBoundsBrush == nullptr
        || m_iconAnchorBrush == nullptr
        || m_iconTextBrush == nullptr
        || m_interactableIconBrush == nullptr
        || m_bumpedIconBrush == nullptr
        || m_iconTextFormat == nullptr) {
        return;
    }

    for (std::size_t index = 0; index < desktopIcons.size(); ++index) {
        const DesktopIcon& icon = desktopIcons[index];
        const D2D1_RECT_F hoverRect = D2D1::RectF(
            static_cast<float>(icon.screenBounds.left - clientScreenOrigin.x),
            static_cast<float>(icon.screenBounds.top - clientScreenOrigin.y),
            static_cast<float>(icon.screenBounds.right - clientScreenOrigin.x),
            static_cast<float>(icon.screenBounds.bottom - clientScreenOrigin.y));

        const D2D1_RECT_F imageRect = D2D1::RectF(
            static_cast<float>(icon.imageBounds.left - clientScreenOrigin.x),
            static_cast<float>(icon.imageBounds.top - clientScreenOrigin.y),
            static_cast<float>(icon.imageBounds.right - clientScreenOrigin.x),
            static_cast<float>(icon.imageBounds.bottom - clientScreenOrigin.y));

        const D2D1_RECT_F labelRect = D2D1::RectF(
            static_cast<float>(icon.labelBounds.left - clientScreenOrigin.x),
            static_cast<float>(icon.labelBounds.top - clientScreenOrigin.y),
            static_cast<float>(icon.labelBounds.right - clientScreenOrigin.x),
            static_cast<float>(icon.labelBounds.bottom - clientScreenOrigin.y));

        const float anchorX = static_cast<float>(icon.anchorPoint.x - clientScreenOrigin.x);
        const float anchorY = static_cast<float>(icon.anchorPoint.y - clientScreenOrigin.y);

        if (settings.showHoverBounds) {
            m_renderTarget->DrawRectangle(hoverRect, m_iconHoverBoundsBrush.Get(), DEBUG_HOVER_STROKE_WIDTH);
        }

        if (settings.showImageBounds) {
            m_renderTarget->DrawRectangle(imageRect, m_iconImageBoundsBrush.Get(), DEBUG_IMAGE_STROKE_WIDTH);
        }

        if (settings.showAnchors) {
            m_renderTarget->DrawLine(
                D2D1::Point2F(anchorX - DEBUG_ANCHOR_MARK_SIZE, anchorY),
                D2D1::Point2F(anchorX + DEBUG_ANCHOR_MARK_SIZE, anchorY),
                m_iconAnchorBrush.Get(),
                DEBUG_IMAGE_STROKE_WIDTH);
            m_renderTarget->DrawLine(
                D2D1::Point2F(anchorX, anchorY - DEBUG_ANCHOR_MARK_SIZE),
                D2D1::Point2F(anchorX, anchorY + DEBUG_ANCHOR_MARK_SIZE),
                m_iconAnchorBrush.Get(),
                DEBUG_IMAGE_STROKE_WIDTH);
        }

        if (settings.showLabels && !icon.displayName.empty()) {
            const D2D1_RECT_F textRect = D2D1::RectF(
                labelRect.left,
                labelRect.top + DEBUG_TEXT_TOP_GAP,
                labelRect.right,
                labelRect.bottom);

            m_renderTarget->DrawTextW(
                icon.displayName.c_str(),
                static_cast<UINT32>(icon.displayName.size()),
                m_iconTextFormat.Get(),
                textRect,
                m_iconTextBrush.Get());
        }

        if (static_cast<int>(index) == interactableIconIndex) {
            m_renderTarget->DrawRectangle(
                hoverRect,
                m_interactableIconBrush.Get(),
                DEBUG_INTERACTABLE_STROKE_WIDTH);
        }

        if (static_cast<int>(index) == bumpedIconIndex) {
            m_renderTarget->DrawRectangle(
                hoverRect,
                m_bumpedIconBrush.Get(),
                DEBUG_BUMPED_STROKE_WIDTH);
        }
    }
}
