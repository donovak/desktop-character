#include "DesktopIconService.h"

#include "DebugLog.h"

#include <dwrite.h>
#include <exdisp.h>
#include <objbase.h>
#include <servprov.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <iterator>
#include <string>

namespace {
constexpr int FALLBACK_ICON_SIZE = 48;
constexpr int FALLBACK_CELL_WIDTH = 75;
constexpr int FALLBACK_CELL_HEIGHT = 90;
constexpr int MIN_LABEL_WIDTH = 60;
constexpr int MAX_LABEL_WIDTH = 82;
constexpr int CELL_LABEL_HORIZONTAL_INSET = 6;
constexpr int DESKTOP_LABEL_LINE_HEIGHT = 16;
constexpr int DESKTOP_MAX_LABEL_LINES = 2;
constexpr int IMAGE_LEFT_OFFSET = 0;
constexpr int IMAGE_TOP_OFFSET = 4;
constexpr int IMAGE_LABEL_GAP = 2;
constexpr int HOVER_HORIZONTAL_PADDING = 2;
constexpr int HOVER_VERTICAL_PADDING = 2;
constexpr float LABEL_FONT_SIZE = 12.0f;

class Pidl {
public:
    explicit Pidl(PITEMID_CHILD pidl)
        : m_pidl(pidl)
    {
    }

    ~Pidl()
    {
        if (m_pidl != nullptr) {
            CoTaskMemFree(m_pidl);
        }
    }

    Pidl(const Pidl&) = delete;
    Pidl& operator=(const Pidl&) = delete;

    PCUITEMID_CHILD get() const
    {
        return m_pidl;
    }

private:
    PITEMID_CHILD m_pidl = nullptr;
};

struct DesktopView {
    Microsoft::WRL::ComPtr<IShellView> shellView;
    Microsoft::WRL::ComPtr<IFolderView> folderView;
    Microsoft::WRL::ComPtr<IFolderView2> folderView2;
    Microsoft::WRL::ComPtr<IShellFolder> shellFolder;
    HWND viewHwnd = nullptr;
};

struct IconLayoutMetrics {
    int iconSize = FALLBACK_ICON_SIZE;
    int cellWidth = FALLBACK_CELL_WIDTH;
    int cellHeight = FALLBACK_CELL_HEIGHT;
    int labelWidth = FALLBACK_CELL_WIDTH - CELL_LABEL_HORIZONTAL_INSET;
};

HRESULT getDesktopView(DesktopView& desktopView)
{
    Microsoft::WRL::ComPtr<IShellWindows> shellWindows;
    HRESULT result = CoCreateInstance(
        CLSID_ShellWindows,
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(shellWindows.GetAddressOf()));

    if (FAILED(result)) {
        return result;
    }

    VARIANT desktopLocation {};
    desktopLocation.vt = VT_I4;
    desktopLocation.lVal = CSIDL_DESKTOP;

    VARIANT empty {};
    empty.vt = VT_EMPTY;

    long shellWindowHwnd = 0;
    Microsoft::WRL::ComPtr<IDispatch> dispatch;
    result = shellWindows->FindWindowSW(
        &desktopLocation,
        &empty,
        SWC_DESKTOP,
        &shellWindowHwnd,
        SWFO_NEEDDISPATCH,
        dispatch.GetAddressOf());

    if (FAILED(result)) {
        return result;
    }

    Microsoft::WRL::ComPtr<IServiceProvider> serviceProvider;
    result = dispatch.As(&serviceProvider);
    if (FAILED(result)) {
        return result;
    }

    Microsoft::WRL::ComPtr<IShellBrowser> shellBrowser;
    result = serviceProvider->QueryService(
        SID_STopLevelBrowser,
        IID_PPV_ARGS(shellBrowser.GetAddressOf()));

    if (FAILED(result)) {
        return result;
    }

    result = shellBrowser->QueryActiveShellView(desktopView.shellView.GetAddressOf());
    if (FAILED(result)) {
        return result;
    }

    result = desktopView.shellView.As(&desktopView.folderView);
    if (FAILED(result)) {
        return result;
    }

    desktopView.shellView.As(&desktopView.folderView2);

    result = desktopView.folderView->GetFolder(
        IID_PPV_ARGS(desktopView.shellFolder.GetAddressOf()));

    if (FAILED(result)) {
        return result;
    }

    desktopView.shellView->GetWindow(&desktopView.viewHwnd);
    return S_OK;
}

IconLayoutMetrics getIconLayoutMetrics(IFolderView* folderView, IFolderView2* folderView2)
{
    IconLayoutMetrics metrics {};

    POINT spacing {};
    if (folderView != nullptr && SUCCEEDED(folderView->GetSpacing(&spacing)) && spacing.x > 0 && spacing.y > 0) {
        metrics.cellWidth = spacing.x;
        metrics.cellHeight = spacing.y;
    } else if (folderView != nullptr && SUCCEEDED(folderView->GetDefaultSpacing(&spacing)) && spacing.x > 0 && spacing.y > 0) {
        metrics.cellWidth = spacing.x;
        metrics.cellHeight = spacing.y;
    }

    FOLDERVIEWMODE viewMode {};
    int iconSize = 0;
    if (folderView2 != nullptr && SUCCEEDED(folderView2->GetViewModeAndIconSize(&viewMode, &iconSize)) && iconSize > 0) {
        metrics.iconSize = iconSize;
    } else {
        const int systemIconSize = GetSystemMetrics(SM_CXICON);
        if (systemIconSize > 0) {
            metrics.iconSize = std::max(FALLBACK_ICON_SIZE, systemIconSize);
        }
    }

    metrics.labelWidth = std::clamp(
        metrics.cellWidth - CELL_LABEL_HORIZONTAL_INSET,
        MIN_LABEL_WIDTH,
        MAX_LABEL_WIDTH);

    return metrics;
}

std::wstring hresultToHex(HRESULT result)
{
    wchar_t buffer[16] {};
    swprintf_s(buffer, L"0x%08X", static_cast<unsigned int>(result));
    return buffer;
}

std::wstring strRetToString(IShellFolder* folder, PCUITEMID_CHILD pidl, SHGDNF flags)
{
    if (folder == nullptr || pidl == nullptr) {
        return {};
    }

    STRRET name {};
    HRESULT result = folder->GetDisplayNameOf(pidl, flags, &name);
    if (FAILED(result)) {
        return {};
    }

    wchar_t buffer[MAX_PATH] {};
    result = StrRetToBufW(&name, pidl, buffer, static_cast<UINT>(std::size(buffer)));
    if (FAILED(result)) {
        return {};
    }

    return buffer;
}

std::wstring displayNameFromPidl(IShellFolder* folder, PCUITEMID_CHILD pidl)
{
    return strRetToString(folder, pidl, SHGDN_NORMAL);
}

std::wstring parsingNameFromPidl(IShellFolder* folder, PCUITEMID_CHILD pidl)
{
    std::wstring name = strRetToString(folder, pidl, SHGDN_FORPARSING);
    if (name.starts_with(L"::")) {
        return {};
    }

    return name;
}

class LabelMeasurer {
public:
    LabelMeasurer()
    {
        HRESULT result = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf()));

        if (FAILED(result)) {
            debugLog(L"DesktopIconService could not create DirectWrite factory; using label length fallback.");
            return;
        }

        result = m_writeFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            LABEL_FONT_SIZE,
            L"",
            m_textFormat.GetAddressOf());

        if (FAILED(result)) {
            debugLog(L"DesktopIconService could not create DirectWrite text format; using label length fallback.");
            m_writeFactory.Reset();
            return;
        }
    }

    int estimateLineCount(const std::wstring& text, int labelWidth) const
    {
        if (text.empty()) {
            return 1;
        }

        if (m_writeFactory == nullptr || m_textFormat == nullptr) {
            return text.size() > 12 ? 2 : 1;
        }

        Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
        const HRESULT result = m_writeFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.size()),
            m_textFormat.Get(),
            static_cast<FLOAT>(labelWidth),
            static_cast<FLOAT>(DESKTOP_LABEL_LINE_HEIGHT * DESKTOP_MAX_LABEL_LINES),
            textLayout.GetAddressOf());

        if (FAILED(result)) {
            return text.size() > 12 ? 2 : 1;
        }

        std::array<DWRITE_LINE_METRICS, DESKTOP_MAX_LABEL_LINES + 1> lineMetrics {};
        UINT32 actualLineCount = 0;
        if (FAILED(textLayout->GetLineMetrics(lineMetrics.data(), static_cast<UINT32>(lineMetrics.size()), &actualLineCount))) {
            return text.size() > 12 ? 2 : 1;
        }

        return std::clamp(static_cast<int>(actualLineCount), 1, DESKTOP_MAX_LABEL_LINES);
    }

private:
    Microsoft::WRL::ComPtr<IDWriteFactory> m_writeFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
};

DesktopIcon estimateIconBounds(POINT anchorPoint, const std::wstring& displayName, const IconLayoutMetrics& metrics, const LabelMeasurer& labelMeasurer)
{
    DesktopIcon icon {};
    icon.displayName = displayName;
    icon.anchorPoint = anchorPoint;
    icon.labelLineCount = labelMeasurer.estimateLineCount(displayName, metrics.labelWidth);

    const int iconLeft = anchorPoint.x + IMAGE_LEFT_OFFSET;
    const int iconTop = anchorPoint.y + IMAGE_TOP_OFFSET;
    icon.imageBounds = {
        iconLeft,
        iconTop,
        iconLeft + metrics.iconSize,
        iconTop + metrics.iconSize
    };

    const int imageCenterX = icon.imageBounds.left + (metrics.iconSize / 2);
    const int labelLeft = imageCenterX - (metrics.labelWidth / 2);
    const int labelTop = icon.imageBounds.bottom + IMAGE_LABEL_GAP;
    icon.labelBounds = {
        labelLeft,
        labelTop,
        labelLeft + metrics.labelWidth,
        labelTop + (icon.labelLineCount * DESKTOP_LABEL_LINE_HEIGHT)
    };

    icon.screenBounds = {
        std::min(icon.imageBounds.left, icon.labelBounds.left) - HOVER_HORIZONTAL_PADDING,
        std::min(icon.imageBounds.top, icon.labelBounds.top) - HOVER_VERTICAL_PADDING,
        std::max(icon.imageBounds.right, icon.labelBounds.right) + HOVER_HORIZONTAL_PADDING,
        std::max(icon.imageBounds.bottom, icon.labelBounds.bottom) + HOVER_VERTICAL_PADDING
    };

    return icon;
}

std::wstring layoutMetricsToString(const IconLayoutMetrics& metrics)
{
    return L"iconSize=" + std::to_wstring(metrics.iconSize)
        + L", cellWidth=" + std::to_wstring(metrics.cellWidth)
        + L", cellHeight=" + std::to_wstring(metrics.cellHeight)
        + L", labelWidth=" + std::to_wstring(metrics.labelWidth);
}
}

DesktopIconService::DesktopIconService() = default;

std::vector<DesktopIcon> DesktopIconService::refresh()
{
    std::vector<DesktopIcon> icons;

    if (!m_comApartment.isInitialized()) {
        debugLog(L"Desktop icon refresh skipped because COM is not initialized.");
        return icons;
    }

    DesktopView desktopView {};
    HRESULT result = getDesktopView(desktopView);
    if (FAILED(result)) {
        debugLog(std::wstring(L"Desktop icon refresh failed: could not get desktop view, hr=") + hresultToHex(result));
        return icons;
    }

    int iconCount = 0;
    result = desktopView.folderView->ItemCount(SVGIO_ALLVIEW, &iconCount);
    if (FAILED(result)) {
        debugLog(std::wstring(L"Desktop icon refresh failed: ItemCount hr=") + hresultToHex(result));
        return icons;
    }

    icons.reserve(static_cast<std::size_t>(iconCount));
    const IconLayoutMetrics metrics = getIconLayoutMetrics(desktopView.folderView.Get(), desktopView.folderView2.Get());
    const LabelMeasurer labelMeasurer;
    debugLog(std::wstring(L"Desktop icon layout metrics: ") + layoutMetricsToString(metrics));

    for (int index = 0; index < iconCount; ++index) {
        PITEMID_CHILD rawPidl = nullptr;
        result = desktopView.folderView->Item(index, &rawPidl);
        if (FAILED(result) || rawPidl == nullptr) {
            continue;
        }

        Pidl pidl(rawPidl);

        POINT itemPosition {};
        result = desktopView.folderView->GetItemPosition(pidl.get(), &itemPosition);
        if (FAILED(result)) {
            continue;
        }

        POINT screenPosition = itemPosition;
        if (desktopView.viewHwnd != nullptr) {
            ClientToScreen(desktopView.viewHwnd, &screenPosition);
        }

        const std::wstring displayName = displayNameFromPidl(desktopView.shellFolder.Get(), pidl.get());
        DesktopIcon icon = estimateIconBounds(screenPosition, displayName, metrics, labelMeasurer);
        icon.filesystemPath = parsingNameFromPidl(desktopView.shellFolder.Get(), pidl.get());
        icon.debugIdentifier = std::wstring(L"desktop-icon-") + std::to_wstring(index);

        icons.push_back(std::move(icon));
    }

    debugLog(std::wstring(L"Desktop icon refresh found ") + std::to_wstring(icons.size()) + L" icons.");
    return icons;
}

DesktopIconService::ComApartment::ComApartment()
{
    m_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    m_shouldUninitialize = SUCCEEDED(m_result);

    if (FAILED(m_result)) {
        debugLog(std::wstring(L"CoInitializeEx failed for DesktopIconService, hr=") + hresultToHex(m_result));
    }
}

DesktopIconService::ComApartment::~ComApartment()
{
    if (m_shouldUninitialize) {
        CoUninitialize();
    }
}

bool DesktopIconService::ComApartment::isInitialized() const
{
    return SUCCEEDED(m_result);
}
