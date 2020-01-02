/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-04-10
    License: MIT
*/

#pragma once

// Fixup some Visual Studio builds not defining this.
#if !defined(_DEBUG) && !defined(NDEBUG)
#define NDEBUG
#endif

// Platform identification.
#if defined(_MSC_VER)
#define EXPORT_ATTR __declspec(dllexport)
#define IMPORT_ATTR __declspec(dllimport)
#elif defined(__GNUC__)
#define EXPORT_ATTR __attribute__((visibility("default")))
#define IMPORT_ATTR
#else
#error Compiling for unknown platform.
#endif

// Remove some Windows annoyance.
#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#endif

// Where to keep the log.
#define LOGPATH "./Ayria/Logs"

// Build information helpers to avoid the preprocessor.
namespace Build
{
    constexpr bool is64bit = sizeof(void *) == 8;

    #if defined(_WIN32)
    constexpr bool isWindows = true;
    #else
    constexpr bool isWindows = false;
    #endif

    #if defined(NDEBUG)
    constexpr bool isDebug = false;
    #else
    constexpr bool isDebug = true;
    #endif
}

// Information logging.
#define Warningprint(string) Logging::Print('W', string)
#define Errorprint(string) Logging::Print('E', string)
#define Infoprint(string) Logging::Print('I', string)
#if !defined(NDEBUG)
#define Debugprint(string) Logging::Print('D', string)
#define Traceprint() Logging::Print('>', __FUNCTION__)
#else
#define Debugprint(string)
#define Traceprint()
#endif

// Helper to support designated initializers until c++ 20 is mainstream.
#define instantiate(T, ...) ([&]{ T ${}; __VA_ARGS__; return $; }())

// Ignore ANSI compatibility for structs.
#pragma warning(disable: 4201)

// Elevate [[nodiscard]] to an error.
#pragma warning(error: 4834)