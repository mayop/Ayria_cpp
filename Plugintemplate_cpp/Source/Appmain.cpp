/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-01-02
    License: MIT
*/

#include "Stdinclude.hpp"

// Optional callbacks when loaded as a plugin.
extern "C"
{
    // NOTE(tcn): All of these are optional, simply comment them out if not needed. See docs for example usage.
    EXPORT_ATTR void onStartup(bool) {}                                             // Do .text edits in this callback.
    EXPORT_ATTR void onInitialized(bool) {}                                         // Do .data edits in this callback.
    EXPORT_ATTR void onReload(void *Previousinstance) {}                            // User-defined, called by devs for hotpatching while developing.
    EXPORT_ATTR bool onEvent(const void *Data, uint32_t Length) { return false; }   // User-defined, returns if the event was handled.
}

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Assets");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);
    }

    return TRUE;
}
#else
__attribute__((constructor)) void DllMain()
{
    // Ensure that Ayrias default directories exist.
    std::filesystem::create_directories("./Ayria/Logs");
    std::filesystem::create_directories("./Ayria/Assets");
    std::filesystem::create_directories("./Ayria/Plugins");

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif