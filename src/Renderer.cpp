#include "Renderer.h"

bool Renderer::initialize(HWND hwnd)
{
    m_hwnd = hwnd;

    const HRESULT factoryResult = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        m_factory.GetAddressOf());

    if (FAILED(factoryResult)) {
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

void Renderer::render(const Character& character)
{
    if (m_renderTarget == nullptr && !createDeviceResources(m_hwnd)) {
        return;
    }

    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(D2D1::ColorF(0.08f, 0.09f, 0.10f));

    const D2D1_RECT_F characterRect = character.bounds();
    m_renderTarget->FillRectangle(characterRect, m_characterBrush.Get());

    const HRESULT result = m_renderTarget->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
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
        return false;
    }

    result = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.14f, 0.73f, 0.48f),
        m_characterBrush.ReleaseAndGetAddressOf());

    if (FAILED(result)) {
        discardDeviceResources();
        return false;
    }

    return true;
}

void Renderer::discardDeviceResources()
{
    m_characterBrush.Reset();
    m_backgroundBrush.Reset();
    m_renderTarget.Reset();
}
