#pragma once
#include <windows.h>
#include <powrprof.h>
#include <string>
#include <stdexcept>

namespace doze {
enum class Action { Shutdown, Restart, Sleep, Hibernate };
inline Action parse_action(const std::string& value) {
    if (value == "shutdown") return Action::Shutdown;
    if (value == "restart") return Action::Restart;
    if (value == "sleep") return Action::Sleep;
    if (value == "hibernate") return Action::Hibernate;
    throw std::invalid_argument("Unknown action");
}
struct Capabilities { bool sleep, hibernate, hibernateSupported, filePresent, modernStandby; };
inline Capabilities capabilities() {
    SYSTEM_POWER_CAPABILITIES p{};
    if (!GetPwrCapabilities(&p)) throw std::runtime_error("GetPwrCapabilities failed");
    // The MinGW header names the Windows 8+ Hiberboot/WakeAlarm/AoAc fields spare2.
    const bool modernStandby = p.spare2[2] != FALSE;
    return {IsPwrSuspendAllowed() != FALSE || modernStandby, IsPwrHibernateAllowed() != FALSE,
            p.SystemS4 != FALSE, p.HiberFilePresent != FALSE, modernStandby};
}
inline DWORD shutdown_privilege() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return GetLastError();
    TOKEN_PRIVILEGES p{}; p.PrivilegeCount = 1;
    if (!LookupPrivilegeValueW(nullptr, L"SeShutdownPrivilege", &p.Privileges[0].Luid)) { auto e=GetLastError(); CloseHandle(token); return e; }
    p.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    BOOL ok = AdjustTokenPrivileges(token, FALSE, &p, 0, nullptr, nullptr);
    DWORD error = GetLastError(); CloseHandle(token);
    return ok ? error : (error ? error : ERROR_ACCESS_DENIED);
}
inline DWORD last_error() { const DWORD error=GetLastError(); return error?error:ERROR_GEN_FAILURE; }
inline DWORD execute(Action action, bool force) {
    DWORD error = shutdown_privilege(); if (error) return error;
    constexpr DWORD reason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED;
    switch (action) {
        case Action::Shutdown: return ExitWindowsEx(EWX_POWEROFF | (force ? EWX_FORCE : 0), reason) ? 0 : last_error();
        case Action::Restart: return ExitWindowsEx(EWX_REBOOT | (force ? EWX_FORCE : 0), reason) ? 0 : last_error();
        case Action::Sleep: return SetSuspendState(FALSE, FALSE, FALSE) ? 0 : last_error();
        case Action::Hibernate: return SetSuspendState(TRUE, FALSE, FALSE) ? 0 : last_error();
    }
    return ERROR_INVALID_PARAMETER;
}
struct Countdown {
    bool active = false, force = false;
    Action action = Action::Shutdown;
    ULONGLONG deadline = 0, seconds = 0;
    void start(Action choice, ULONGLONG duration, bool forced, ULONGLONG now) {
        if (active) throw std::invalid_argument("Countdown already active");
        if (!duration || duration > 359999) throw std::invalid_argument("Invalid duration");
        action=choice; force=forced; seconds=duration; deadline=now+duration*1000; active=true;
    }
    ULONGLONG remaining(ULONGLONG now) const { return !active || now >= deadline ? 0 : (deadline-now+999)/1000; }
    bool due(ULONGLONG now) { if (!active || now < deadline) return false; active=false; return true; }
    void cancel() { active=false; }
};
}
