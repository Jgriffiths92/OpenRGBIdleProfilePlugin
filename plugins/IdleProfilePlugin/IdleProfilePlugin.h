#pragma once

#include "OpenRGBPluginInterface.h"
#include <QObject>
#include <QMenu>
#include <QMutex>
#include <QWidget>
#include <QStringList>
#include <thread>
#include <atomic>
#include <QString>
#include <cstdint>

class IdleProfileWidget;

class IdleProfilePlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.OpenRGBPluginInterface")
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    ~IdleProfilePlugin() override;

    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override;

    void Load(ResourceManagerInterface* resource_manager_ptr) override;
    QWidget* GetWidget() override;
    QMenu* GetTrayMenu() override;
    void Unload() override;

    void LoadSettings();
    void SaveSettings();
    QStringList GetAvailableProfiles();

    int GetIdleSeconds();
    QString GetIdleProfile();
    QString GetActiveProfile();
    bool GetEnabled();
    bool GetApplyActiveOnStart();
    int GetResumeCooldownSeconds();
    bool GetDetectScreenOff();
    bool GetApplyActiveOnScreenOn();
    bool GetPauseIdleWhileMediaPlaying();
    bool GetDebugLogging();
    bool GetIsCurrentlyIdle();
    qint64 GetLastSwitchEpochMs();

    void SetIdleSeconds(int seconds);
    void SetIdleProfile(const QString& profile);
    void SetActiveProfile(const QString& profile);
    void SetEnabled(bool is_enabled);
    void SetApplyActiveOnStart(bool value);
    void SetResumeCooldownSeconds(int seconds);
    void SetDetectScreenOff(bool value);
    void SetApplyActiveOnScreenOn(bool value);
    void SetPauseIdleWhileMediaPlaying(bool value);
    void SetDebugLogging(bool value);

    // settings (shared with UI)
    int idle_seconds = 600;
    QString idle_profile;
    QString active_profile;
    bool enabled = true;
    bool apply_active_on_start = false;
    int resume_cooldown_seconds = 3;
    bool detect_screen_off = true;
    bool apply_active_on_screen_on = false;
    bool pause_idle_while_media_playing = true;
    bool debug_logging = false;

private:
    void Run();
    void SetRuntimeState(bool idle_state);
    void DebugLog(const QString& message);

    ResourceManagerInterface* resource_manager = nullptr;
    IdleProfileWidget* widget = nullptr;
    QMutex settings_mutex;
    bool currently_idle = false;
    qint64 last_switch_epoch_ms = 0;

    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<bool> loaded{false};
};