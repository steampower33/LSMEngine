#pragma once

#include <stdexcept>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

inline std::wstring GetAssetsPath()
{
    WCHAR path[512];
    DWORD size = GetModuleFileName(nullptr, path, _countof(path));
    if (size == 0 || size >= _countof(path))
    {
        throw std::runtime_error("Failed to get module file name.");
    }

    std::wstring assetsPath(path);
    size_t lastSlashPos = assetsPath.find_last_of(L'\\');
    if (lastSlashPos != std::wstring::npos)
    {
        assetsPath = assetsPath.substr(0, lastSlashPos + 1);
    }

    return assetsPath;
}
