#include "IdleProfilePlugin.h"
#include "IdleProfileWidget.h"
#include "filesystem.h"

#include <QCoreApplication>
#include <QImage>
#include <QMutexLocker>
#include <QSet>
#ifdef _WIN32
#include <windows.h>
#endif
#include <chrono>
#include <thread>

#include <QProcess>
#include <QSettings>
#include <QString>
#include <QDateTime>
#include <QDebug>

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

    while (running)
    {
        int idle_seconds_local;
        int cooldown_seconds_local;
        QString idle_profile_local;
        QString active_profile_local;
        bool enabled_local;
        bool debug_logging_local;

        {
            QMutexLocker locker(&settings_mutex);
            idle_seconds_local = idle_seconds;
            cooldown_seconds_local = resume_cooldown_seconds;
            idle_profile_local = idle_profile;
            active_profile_local = active_profile;
            enabled_local = enabled;
            debug_logging_local = debug_logging;
        }

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
        if (!isIdle && idleTime >= (DWORD)idle_seconds_local * 1000 && cooldown_passed)
        {
            if (!idle_profile_local.isEmpty())
            {
                LoadProfile(idle_profile_local);
                if(debug_logging_local)
                {
                    DebugLog(QString("Switched to idle profile: %1").arg(idle_profile_local));
                }
            }

            isIdle = true;
            SetRuntimeState(true);
            last_transition = now;
        }

        // ---------------------------
        // Return to active
        // ---------------------------
        if (isIdle && idleTime < 2000 && cooldown_passed)
        {
            if (!active_profile_local.isEmpty())
            {
                LoadProfile(active_profile_local);
                if(debug_logging_local)
                {
                    DebugLog(QString("Returned to active profile: %1").arg(active_profile_local));
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

void IdleProfilePlugin::SetDebugLogging(bool value)
{
    QMutexLocker locker(&settings_mutex);
    debug_logging = value;
}