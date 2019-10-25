/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2019-10-20
    License: MIT
*/

#include "Stdinclude.hpp"

namespace HWBP
{
    enum Access : uint8_t { REMOVE = 0, READ = 1, WRITE = 2, EXEC = 4 };

    namespace Internal
    {
        struct Context_t
        {
            uintptr_t Address;
            HANDLE Threadhandle;
            uint8_t Accessflags;
            uint8_t Size;
        };
        DWORD __stdcall Thread(void *Param)
        {
            Context_t *Context = static_cast<Context_t *>(Param);
            CONTEXT Threadcontext{ CONTEXT_DEBUG_REGISTERS };

            // MSDN says thread-context on running threads is UB.
            if(DWORD(-1) == SuspendThread(Context->Threadhandle))
                return GetLastError();

            // Get the current debug-registers.
            if (!GetThreadContext(Context->Threadhandle, &Threadcontext))
                return GetLastError();

            // Special case: Removal of a breakpoint.
            if (Context->Accessflags == Access::REMOVE)
            {
                do
                {
                    if (Context->Address == Threadcontext.Dr0)
                    {
                        Threadcontext.Dr7 &= ~1;
                        Threadcontext.Dr0 = 0;
                        break;
                    }
                    if (Context->Address == Threadcontext.Dr1)
                    {
                        Threadcontext.Dr7 &= ~4;
                        Threadcontext.Dr1 = 0;
                        break;
                    }
                    if (Context->Address == Threadcontext.Dr2)
                    {
                        Threadcontext.Dr7 &= ~16;
                        Threadcontext.Dr2 = 0;
                        break;
                    }
                    if (Context->Address == Threadcontext.Dr3)
                    {
                        Threadcontext.Dr7 &= ~64;
                        Threadcontext.Dr3 = 0;
                        break;
                    }

                    // Nothing to do..
                    return 0;
                } while (false);

                Threadcontext.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                SetThreadContext(Context->Threadhandle, &Threadcontext);
                ResumeThread(Context->Threadhandle);
                return 0;
            }

            // Do we even have any available global registers?
            if (Threadcontext.Dr7 & 0x55) return DWORD(-1);

            // Convert the flags to x86-global.
            uint8_t Accessflags = (Context->Accessflags & Access::WRITE * 3) | (Context->Accessflags & Access::READ);
            uint8_t Sizeflags = 3 & (Context->Size % 8 == 0 ? 2 : Context->Size - 1);

            // Find the empty register and set it.
            for (int i = 0; i < 4; ++i)
            {
                if (!(Threadcontext.Dr7 & (1 << (i * 2))))
                {
                    #define Setbits(Register, Low, Count, Value) \
                    Register = (Register & ~(((1 << (Count)) - 1) << (Low)) | ((Value) << (Low)));
                    Setbits(Threadcontext.Dr7, 16 + i * 4, 2, Accessflags);
                    Setbits(Threadcontext.Dr7, 18 + i * 4, 2, Sizeflags);
                    Setbits(Threadcontext.Dr7, i * 2, 1, 1);
                    #undef Setbits

                    Threadcontext.Dr0 = i == 0 ? Context->Address : Threadcontext.Dr0;
                    Threadcontext.Dr1 = i == 1 ? Context->Address : Threadcontext.Dr1;
                    Threadcontext.Dr2 = i == 2 ? Context->Address : Threadcontext.Dr2;
                    Threadcontext.Dr3 = i == 3 ? Context->Address : Threadcontext.Dr3;

                    Threadcontext.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    SetThreadContext(Context->Threadhandle, &Threadcontext);
                    ResumeThread(Context->Threadhandle);
                    return 0;
                }
            }

            // WTF?
            return DWORD(-1);
        }
    }

    DWORD Set(HANDLE Threadhandle, uintptr_t Address, uint8_t Accessflags = Access::EXEC, uint8_t Size = 1)
    {
        Internal::Context_t Context{ Address, Threadhandle, Accessflags, Size };

        // Ensure that we have access rights.
        if (Context.Threadhandle == GetCurrentThread())
            Context.Threadhandle = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT |
                                      THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION,
                                      FALSE, GetCurrentThreadId());

        // We need to suspend the current thread, so create a new one.
        auto Localthread = CreateThread(0, 0, Internal::Thread, &Context, 0, 0);
        if(!Localthread) return GetLastError();

        DWORD Result{}; // Wait for the thread to finish.
        while (!GetExitCodeThread(Localthread, &Result)) Sleep(0);

        // Close the handle if we opened it.
        if (Context.Threadhandle == GetCurrentThread())
            CloseHandle(Context.Threadhandle);

        return Result;
    }
    DWORD Set(uintptr_t Address, uint8_t Accessflags = Access::EXEC, uint8_t Size = 1)
    {
        return Set(GetCurrentThread(), Address, Accessflags, Size);
    }
    DWORD Remove(HANDLE Threadhandle, uintptr_t Address)
    {
        return Set(Threadhandle, Address, Access::REMOVE);
    }
    DWORD Remove(uintptr_t Address)
    {
        return Remove(GetCurrentThread(), Address);
    }
}



// Entrypoint when loaded as a plugin.
extern "C"
{
    EXPORT_ATTR void onStartup(bool)
    {
    }
    EXPORT_ATTR void onInitialized(bool) { /* Do .data edits */ }
    EXPORT_ATTR bool onMessage(const void *, uint32_t) { return false; }
}

// Entrypoint when loaded as a shared library.
#if defined _WIN32
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        _mkdir("./Ayria/");
        _mkdir("./Ayria/Plugins/");
        _mkdir("./Ayria/Assets/");
        _mkdir("./Ayria/Local/");
        _mkdir("./Ayria/Logs/");

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
    mkdir("./Ayria/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Plugins/", S_IRWXU | S_IRWXG);
    mkdir("./Ayria/Assets/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Local/", S_IRWU | S_IRWG);
    mkdir("./Ayria/Logs/", S_IRWU | S_IRWG);

    // Only keep a log for this session.
    Logging::Clearlog();
}
#endif
