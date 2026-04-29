#include "DesktopIconService.h"

#include "DebugLog.h"

#include <objbase.h>
#include <exdisp.h>
#include <servprov.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wrl/client.h>

#include <cwchar>
#include <iterator>
#include <string>

namespace {
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
    Microsoft::WRL::ComPtr<IShellFolder> shellFolder;
    HWND viewHwnd = nullptr;
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

    result = desktopView.folderView->GetFolder(
        IID_PPV_ARGS(desktopView.shellFolder.GetAddressOf()));

    if (FAILED(result)) {
        return result;
    }

    desktopView.shellView->GetWindow(&desktopView.viewHwnd);
    return S_OK;
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

RECT estimateIconBounds(POINT screenPosition)
{
    const int width = GetSystemMetrics(SM_CXICONSPACING);
    const int height = GetSystemMetrics(SM_CYICONSPACING);

    return {
        screenPosition.x,
        screenPosition.y,
        screenPosition.x + width,
        screenPosition.y + height
    };
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

        DesktopIcon icon {};
        icon.displayName = displayNameFromPidl(desktopView.shellFolder.Get(), pidl.get());
        icon.filesystemPath = parsingNameFromPidl(desktopView.shellFolder.Get(), pidl.get());
        icon.debugIdentifier = std::wstring(L"desktop-icon-") + std::to_wstring(index);
        icon.screenBounds = estimateIconBounds(screenPosition);

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
