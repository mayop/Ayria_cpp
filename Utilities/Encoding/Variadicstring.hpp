/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-03-14
    License: MIT
*/

#pragma once
#include <memory>
#include <string_view>

#if defined(HAS_ABSEIL)
template<typename ... Args> [[nodiscard]] std::string va(std::string_view Format, Args ...args)
{
    return absl::StrFormat(Format, args ...);
}
template<typename ... Args> [[nodiscard]] std::string va(const char *Format, Args ...args)
{
    return absl::StrFormat(Format, args ...);
}
#else
template<typename ... Args> [[nodiscard]] std::string va(std::string_view Format, Args ...args)
{
    const auto Size = std::snprintf(nullptr, 0, Format.data(), args ...);
    auto Buffer = std::make_unique<char[]>(Size + 1);

    std::snprintf(Buffer.get(), Size, Format.data(), args ...);
    return Buffer.get();
}
template<typename ... Args> [[nodiscard]] std::string va(const char *Format, Args ...args)
{
    const auto Size = std::snprintf(nullptr, 0, Format, args ...);
    auto Buffer = std::make_unique<char[]>(Size + 1);

    std::snprintf(Buffer.get(), Size, Format, args ...);
    return Buffer.get();
}
#endif

#if defined (_WIN32)
template<typename ... Args> [[nodiscard]] std::wstring va(std::wstring_view Format, Args ...args)
{
    const auto Size = _snwprintf(nullptr, 0, Format.data(), args ...);
    auto Buffer = std::make_unique<wchar_t[]>(Size + 1);

    _snwprintf(Buffer.get(), Size, Format.data(), args ...);
    return Buffer.get();
}
template<typename ... Args> [[nodiscard]] std::wstring va(const wchar_t *Format, Args ...args)
{
    const auto Size = _snwprintf(nullptr, 0, Format, args ...);
    auto Buffer = std::make_unique<wchar_t[]>(Size + 1);

    _snwprintf(Buffer.get(), Size, Format, args ...);
    return Buffer.get();
}
#endif
