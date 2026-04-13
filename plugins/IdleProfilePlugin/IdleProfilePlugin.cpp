#include "IdleProfilePlugin.h"
#include "IdleProfileWidget.h"
#include "filesystem.h"

#include <QCoreApplication>
#include <QImage>
#include <QMutexLocker>
#include <QSet>
#ifdef _WIN32
#include <windows.h>
#include <dbt.h>
#endif
#include <chrono>
#include <thread>

#include <QProcess>
#include <QSettings>
#include <QString>
#include <QDateTime>
#include <QDebug>

#ifdef _WIN32
// Tracks console display power state using a hidden message-only window.
class DisplayStateWatcher
{
public:
    ~DisplayStateWatcher()
    {
        Shutdown();
    }

    bool Initialize()
    {
        const wchar_t* class_name = L"IdleProfilePluginDisplayStateWatcherWindow";

        WNDCLASSW wc = {};
        wc.lpfnWndProc = &DisplayStateWatcher::WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = class_name;

        RegisterClassW(&wc);

        hwnd = CreateWindowExW(
            0,
            class_name,
            L"",
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            nullptr,
            wc.hInstance,
            this
        );

        if(hwnd == nullptr)
        {
            return false;
        }

        notify_console_display_state_handle = RegisterPowerSettingNotification(hwnd, &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_WINDOW_HANDLE);
        notify_monitor_power_handle = RegisterPowerSettingNotification(hwnd, &GUID_MONITOR_POWER_ON, DEVICE_NOTIFY_WINDOW_HANDLE);

        if(notify_console_display_state_handle == nullptr && notify_monitor_power_handle == nullptr)
        {
            DestroyWindow(hwnd);
            hwnd = nullptr;
            return false;
        }

        return true;
    }

    void PumpMessages()
    {
        if(hwnd == nullptr)
        {
            return;
        }

        MSG msg;
        while(PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    bool IsDisplayOff() const
    {
        return display_off;
    }

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if(msg == WM_NCCREATE)
        {
            const auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create_struct->lpCreateParams));
            return TRUE;
        }

        auto* self = reinterpret_cast<DisplayStateWatcher*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if(self == nullptr)
        {
            return DefWindowProcW(window, msg, wparam, lparam);
        }

        if(msg == WM_POWERBROADCAST && wparam == PBT_POWERSETTINGCHANGE)
        {
            const auto* setting = reinterpret_cast<POWERBROADCAST_SETTING*>(lparam);

            if(setting != nullptr && IsEqualGUID(setting->PowerSetting, GUID_CONSOLE_DISPLAY_STATE) && setting->DataLength >= sizeof(DWORD))
            {
                const DWORD state = *reinterpret_cast<const DWORD*>(setting->Data);
                self->display_off = (state == 0);
                return TRUE;
            }

            if(setting != nullptr && IsEqualGUID(setting->PowerSetting, GUID_MONITOR_POWER_ON) && setting->DataLength >= sizeof(DWORD))
            {
                const DWORD monitor_on = *reinterpret_cast<const DWORD*>(setting->Data);
                self->display_off = (monitor_on == 0);
                return TRUE;
            }
        }

        return DefWindowProcW(window, msg, wparam, lparam);
    }

    void Shutdown()
    {
        if(notify_console_display_state_handle != nullptr)
        {
            UnregisterPowerSettingNotification(notify_console_display_state_handle);
            notify_console_display_state_handle = nullptr;
        }

        if(notify_monitor_power_handle != nullptr)
        {
            UnregisterPowerSettingNotification(notify_monitor_power_handle);
            notify_monitor_power_handle = nullptr;
        }

        if(hwnd != nullptr)
        {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }
    }

    HWND hwnd = nullptr;
    HPOWERNOTIFY notify_console_display_state_handle = nullptr;
    HPOWERNOTIFY notify_monitor_power_handle = nullptr;
    bool display_off = false;
};
#endif

// ---------------------------
// Idle detection (Windows API)
// ---------------------------
DWORD GetIdleTime()
{
#ifdef _WIN32
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);
    return GetTickCount() - lii.dwTime;
#else
    return 0;
#endif
}

// ---------------------------
// Safe profile loader (VERSION-SAFE)
// Uses OpenRGB CLI interface via Qt (stable across versions)
// ---------------------------
static void LoadProfile(const QString& profile)
{
    const QString exe = QCoreApplication::applicationFilePath();

    QProcess::execute(exe, QStringList() << "--profile" << profile);
}

// ---------------------------
// DEFAULTS
// ---------------------------
static const int DEFAULT_IDLE_SECONDS = 600; // 10 min
static const int DEFAULT_RESUME_COOLDOWN_SECONDS = 3;

IdleProfilePlugin::~IdleProfilePlugin()
{
    Unload();
}

// ---------------------------
// Plugin lifecycle
// ---------------------------
OpenRGBPluginInfo IdleProfilePlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info;
    info.Name = "Idle Profile Plugin";
    info.Description = "Switches OpenRGB profiles based on user idle state";
    info.Version = "0.1.0";
    info.Commit = "local";
    info.URL = "https://openrgb.org";
    info.Icon = QImage();
    info.Location = OPENRGB_PLUGIN_LOCATION_TOP;
    info.Label = "Idle Profile";
    info.TabIconString = "";
    info.TabIcon = QImage();
    return info;
}

unsigned int IdleProfilePlugin::GetPluginAPIVersion()
{
    return OPENRGB_PLUGIN_API_VERSION;
}

void IdleProfilePlugin::Load(ResourceManagerInterface* resource_manager_ptr)
{
    if(loaded)
    {
        return;
    }

    resource_manager = resource_manager_ptr;
    LoadSettings();

    if(GetApplyActiveOnStart())
    {
        const QString startup_profile = GetActiveProfile();
        if(!startup_profile.isEmpty())
        {
            LoadProfile(startup_profile);
            SetRuntimeState(false);
            DebugLog(QString("Applied active profile at startup: %1").arg(startup_profile));
        }
    }

    running = true;
    worker = std::thread(&IdleProfilePlugin::Run, this);
    loaded = true;
}

QWidget* IdleProfilePlugin::GetWidget()
{
    if(widget == nullptr)
    {
        widget = new IdleProfileWidget(this);
    }

    return widget;
}

QMenu* IdleProfilePlugin::GetTrayMenu()
{
    return nullptr;
}

void IdleProfilePlugin::Unload()
{
    if(!loaded)
    {
        return;
    }

    running = false;

    if (worker.joinable())
        worker.join();

    if(widget != nullptr)
    {
        delete widget;
        widget = nullptr;
    }

    SaveSettings();
    resource_manager = nullptr;
    loaded = false;
}

// ---------------------------
// MAIN LOOP
// ---------------------------
void IdleProfilePlugin::Run()
{
    bool isIdle = false;
    auto last_transition = std::chrono::steady_clock::now() - std::chrono::hours(1);

#ifdef _WIN32
    DisplayStateWatcher display_state_watcher;
    const bool has_display_state_watcher = display_state_watcher.Initialize();
    bool previous_display_is_off = false;
#endif

    while (running)
    {
        int idle_seconds_local;
        int cooldown_seconds_local;
        QString idle_profile_local;
        QString active_profile_local;
        bool enabled_local;
        bool detect_screen_off_local;
        bool apply_active_on_screen_on_local;
        bool debug_logging_local;

        {
            QMutexLocker locker(&settings_mutex);
            idle_seconds_local = idle_seconds;
            cooldown_seconds_local = resume_cooldown_seconds;
            idle_profile_local = idle_profile;
            active_profile_local = active_profile;
            enabled_local = enabled;
            detect_screen_off_local = detect_screen_off;
            apply_active_on_screen_on_local = apply_active_on_screen_on;
            debug_logging_local = debug_logging;
        }

#ifdef _WIN32
        bool display_is_off = false;
        bool screen_turned_on = false;
        if(has_display_state_watcher)
        {
            display_state_watcher.PumpMessages();
            display_is_off = display_state_watcher.IsDisplayOff();
            screen_turned_on = previous_display_is_off && !display_is_off;
            previous_display_is_off = display_is_off;
        }
#else
        const bool display_is_off = false;
        const bool screen_turned_on = false;
#endif

        if (!enabled_local)
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        DWORD idleTime = GetIdleTime();
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_transition).count();
        const bool cooldown_passed = elapsed >= cooldown_seconds_local;

        // ---------------------------
        // Enter idle
        // ---------------------------
        const bool screen_off_idle_trigger = detect_screen_off_local && display_is_off;

        if (!isIdle && (idleTime >= (DWORD)idle_seconds_local * 1000 || screen_off_idle_trigger) && cooldown_passed)
        {
            if (!idle_profile_local.isEmpty())
            {
                LoadProfile(idle_profile_local);
                if(debug_logging_local)
                {
                    if(screen_off_idle_trigger)
                    {
                        DebugLog(QString("Switched to idle profile (screen off): %1").arg(idle_profile_local));
                    }
                    else
                    {
                        DebugLog(QString("Switched to idle profile: %1").arg(idle_profile_local));
                    }
                }
            }

            isIdle = true;
            SetRuntimeState(true);
            last_transition = now;
        }

        // ---------------------------
        // Return to active
        // ---------------------------
        const bool wake_idle_trigger = apply_active_on_screen_on_local && screen_turned_on;

        if (isIdle && (idleTime < 2000 || wake_idle_trigger) && cooldown_passed)
        {
            if (!active_profile_local.isEmpty())
            {
                LoadProfile(active_profile_local);
                if(debug_logging_local)
                {
                    if(wake_idle_trigger)
                    {
                        DebugLog(QString("Returned to active profile (screen on): %1").arg(active_profile_local));
                    }
                    else
                    {
                        DebugLog(QString("Returned to active profile: %1").arg(active_profile_local));
                    }
                }
            }

            isIdle = false;
            SetRuntimeState(false);
            last_transition = now;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

// ---------------------------
// SETTINGS (version-safe via QSettings)
// ---------------------------
void IdleProfilePlugin::LoadSettings()
{
    QSettings s("OpenRGB", "IdleProfilePlugin");

    QMutexLocker locker(&settings_mutex);
    idle_seconds = s.value("idle_seconds", DEFAULT_IDLE_SECONDS).toInt();
    idle_profile = s.value("idle_profile", "").toString();
    active_profile = s.value("active_profile", "").toString();
    enabled = s.value("enabled", true).toBool();
    apply_active_on_start = s.value("apply_active_on_start", false).toBool();
    resume_cooldown_seconds = s.value("resume_cooldown_seconds", DEFAULT_RESUME_COOLDOWN_SECONDS).toInt();
    detect_screen_off = s.value("detect_screen_off", true).toBool();
    apply_active_on_screen_on = s.value("apply_active_on_screen_on", false).toBool();
    debug_logging = s.value("debug_logging", false).toBool();
}

void IdleProfilePlugin::SaveSettings()
{
    QSettings s("OpenRGB", "IdleProfilePlugin");

    QMutexLocker locker(&settings_mutex);
    s.setValue("idle_seconds", idle_seconds);
    s.setValue("idle_profile", idle_profile);
    s.setValue("active_profile", active_profile);
    s.setValue("enabled", enabled);
    s.setValue("apply_active_on_start", apply_active_on_start);
    s.setValue("resume_cooldown_seconds", resume_cooldown_seconds);
    s.setValue("detect_screen_off", detect_screen_off);
    s.setValue("apply_active_on_screen_on", apply_active_on_screen_on);
    s.setValue("debug_logging", debug_logging);
}

void IdleProfilePlugin::SetRuntimeState(bool idle_state)
{
    QMutexLocker locker(&settings_mutex);
    currently_idle = idle_state;
    last_switch_epoch_ms = QDateTime::currentMSecsSinceEpoch();
}

void IdleProfilePlugin::DebugLog(const QString& message)
{
    {
        QMutexLocker locker(&settings_mutex);
        if(!debug_logging)
        {
            return;
        }
    }

    qInfo().noquote() << QString("[IdleProfilePlugin] %1").arg(message);
}

QStringList IdleProfilePlugin::GetAvailableProfiles()
{
    if(resource_manager == nullptr)
    {
        return QStringList();
    }

    QStringList profiles;
    QSet<QString> dedupe;

    const filesystem::path config_dir = resource_manager->GetConfigurationDirectory();

    if(!filesystem::is_directory(config_dir))
    {
        return profiles;
    }

    for(const auto& entry : filesystem::directory_iterator(config_dir))
    {
        if(!entry.is_regular_file())
        {
            continue;
        }

        const filesystem::path path = entry.path();
        if(path.extension() != ".orp")
        {
            continue;
        }

        const QString qname = QString::fromStdString(path.stem().string());
        if(!qname.isEmpty() && !dedupe.contains(qname))
        {
            dedupe.insert(qname);
            profiles.append(qname);
        }
    }

    profiles.sort(Qt::CaseInsensitive);
    return profiles;
}

int IdleProfilePlugin::GetIdleSeconds()
{
    QMutexLocker locker(&settings_mutex);
    return idle_seconds;
}

QString IdleProfilePlugin::GetIdleProfile()
{
    QMutexLocker locker(&settings_mutex);
    return idle_profile;
}

QString IdleProfilePlugin::GetActiveProfile()
{
    QMutexLocker locker(&settings_mutex);
    return active_profile;
}

bool IdleProfilePlugin::GetEnabled()
{
    QMutexLocker locker(&settings_mutex);
    return enabled;
}

bool IdleProfilePlugin::GetApplyActiveOnStart()
{
    QMutexLocker locker(&settings_mutex);
    return apply_active_on_start;
}

int IdleProfilePlugin::GetResumeCooldownSeconds()
{
    QMutexLocker locker(&settings_mutex);
    return resume_cooldown_seconds;
}

bool IdleProfilePlugin::GetDetectScreenOff()
{
    QMutexLocker locker(&settings_mutex);
    return detect_screen_off;
}

bool IdleProfilePlugin::GetApplyActiveOnScreenOn()
{
    QMutexLocker locker(&settings_mutex);
    return apply_active_on_screen_on;
}

bool IdleProfilePlugin::GetDebugLogging()
{
    QMutexLocker locker(&settings_mutex);
    return debug_logging;
}

bool IdleProfilePlugin::GetIsCurrentlyIdle()
{
    QMutexLocker locker(&settings_mutex);
    return currently_idle;
}

qint64 IdleProfilePlugin::GetLastSwitchEpochMs()
{
    QMutexLocker locker(&settings_mutex);
    return last_switch_epoch_ms;
}

void IdleProfilePlugin::SetIdleSeconds(int seconds)
{
    QMutexLocker locker(&settings_mutex);
    idle_seconds = seconds;
}

void IdleProfilePlugin::SetIdleProfile(const QString& profile)
{
    QMutexLocker locker(&settings_mutex);
    idle_profile = profile;
}

void IdleProfilePlugin::SetActiveProfile(const QString& profile)
{
    QMutexLocker locker(&settings_mutex);
    active_profile = profile;
}

void IdleProfilePlugin::SetEnabled(bool is_enabled)
{
    QMutexLocker locker(&settings_mutex);
    enabled = is_enabled;
}

void IdleProfilePlugin::SetApplyActiveOnStart(bool value)
{
    QMutexLocker locker(&settings_mutex);
    apply_active_on_start = value;
}

void IdleProfilePlugin::SetResumeCooldownSeconds(int seconds)
{
    QMutexLocker locker(&settings_mutex);
    resume_cooldown_seconds = seconds;
}

void IdleProfilePlugin::SetDetectScreenOff(bool value)
{
    QMutexLocker locker(&settings_mutex);
    detect_screen_off = value;
}

void IdleProfilePlugin::SetApplyActiveOnScreenOn(bool value)
{
    QMutexLocker locker(&settings_mutex);
    apply_active_on_screen_on = value;
}

void IdleProfilePlugin::SetDebugLogging(bool value)
{
    QMutexLocker locker(&settings_mutex);
    debug_logging = value;
}