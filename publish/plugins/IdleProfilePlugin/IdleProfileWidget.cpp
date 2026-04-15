#include "IdleProfileWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QDateTime>

IdleProfileWidget::IdleProfileWidget(IdleProfilePlugin* p, QWidget* parent)
    : QWidget(parent), plugin(p)
{
    auto* layout = new QVBoxLayout(this);

    enabledBox = new QCheckBox("Enable Idle RGB Switching");
    layout->addWidget(enabledBox);

    applyActiveOnStartBox = new QCheckBox("Apply active profile on OpenRGB launch");
    layout->addWidget(applyActiveOnStartBox);

    detectScreenOffBox = new QCheckBox("Treat screen off as idle");
    layout->addWidget(detectScreenOffBox);

    treatLockScreenAsIdleBox = new QCheckBox("Treat lock screen as instant idle");
    layout->addWidget(treatLockScreenAsIdleBox);

    applyActiveOnUnlockBox = new QCheckBox("Restore active profile when session unlocks");
    layout->addWidget(applyActiveOnUnlockBox);

    applyActiveOnScreenOnBox = new QCheckBox("Restore active profile when screen turns on");
    layout->addWidget(applyActiveOnScreenOnBox);

    auto* screenStateNoteLabel = new QLabel("Note: Screen power detection may not work with all monitors.");
    screenStateNoteLabel->setWordWrap(true);
    layout->addWidget(screenStateNoteLabel);

    pauseIdleWhileMediaPlayingBox = new QCheckBox("Do not trigger idle while media is playing");
    layout->addWidget(pauseIdleWhileMediaPlayingBox);

    debugLoggingBox = new QCheckBox("Enable debug logging");
    layout->addWidget(debugLoggingBox);

    layout->addWidget(new QLabel("Idle time (seconds):"));
    idleTime = new QSpinBox();
    idleTime->setRange(10, 7200);
    layout->addWidget(idleTime);

    layout->addWidget(new QLabel("Idle profile:"));
    idleProfileBox = new QComboBox();
    idleProfileBox->setEditable(true);
    idleProfileBox->setInsertPolicy(QComboBox::NoInsert);
    layout->addWidget(idleProfileBox);

    layout->addWidget(new QLabel("Active profile:"));
    activeProfileBox = new QComboBox();
    activeProfileBox->setEditable(true);
    activeProfileBox->setInsertPolicy(QComboBox::NoInsert);
    layout->addWidget(activeProfileBox);

    layout->addWidget(new QLabel("Resume cooldown (seconds):"));
    cooldownSeconds = new QSpinBox();
    cooldownSeconds->setRange(0, 60);
    layout->addWidget(cooldownSeconds);

    refreshProfilesButton = new QPushButton("Refresh Profiles");
    layout->addWidget(refreshProfilesButton);

    stateLabel = new QLabel("State: Active");
    layout->addWidget(stateLabel);

    lastSwitchLabel = new QLabel("Last switch: Never");
    layout->addWidget(lastSwitchLabel);

    statusTimer = new QTimer(this);
    statusTimer->setInterval(1000);

    // bind values
    RefreshProfiles();
    RefreshUI();
    RefreshStatus();

    connect(enabledBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(applyActiveOnStartBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(detectScreenOffBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(treatLockScreenAsIdleBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(applyActiveOnUnlockBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(applyActiveOnScreenOnBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(pauseIdleWhileMediaPlayingBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(debugLoggingBox, &QCheckBox::toggled, this, &IdleProfileWidget::Save);
    connect(idleTime, qOverload<int>(&QSpinBox::valueChanged), this, &IdleProfileWidget::Save);
    connect(cooldownSeconds, qOverload<int>(&QSpinBox::valueChanged), this, &IdleProfileWidget::Save);
    connect(idleProfileBox, &QComboBox::editTextChanged, this, &IdleProfileWidget::Save);
    connect(activeProfileBox, &QComboBox::editTextChanged, this, &IdleProfileWidget::Save);
    connect(refreshProfilesButton, &QPushButton::clicked, this, [this]()
    {
        RefreshProfiles();
        RefreshUI();
    });
    connect(statusTimer, &QTimer::timeout, this, &IdleProfileWidget::RefreshStatus);
    statusTimer->start();
}

void IdleProfileWidget::showEvent(QShowEvent* event)
{
    RefreshProfiles();
    RefreshUI();
    RefreshStatus();
    QWidget::showEvent(event);
}

void IdleProfileWidget::RefreshProfiles()
{
    const QString idleSelection = plugin->GetIdleProfile();
    const QString activeSelection = plugin->GetActiveProfile();
    const QStringList profiles = plugin->GetAvailableProfiles();

    {
        QSignalBlocker idleBlocker(idleProfileBox);
        idleProfileBox->clear();
        idleProfileBox->addItems(profiles);
        idleProfileBox->setEditText(idleSelection);
    }

    {
        QSignalBlocker activeBlocker(activeProfileBox);
        activeProfileBox->clear();
        activeProfileBox->addItems(profiles);
        activeProfileBox->setEditText(activeSelection);
    }
}

void IdleProfileWidget::RefreshUI()
{
    QSignalBlocker enabledBlocker(enabledBox);
    QSignalBlocker applyAtStartupBlocker(applyActiveOnStartBox);
    QSignalBlocker detectScreenOffBlocker(detectScreenOffBox);
    QSignalBlocker treatLockScreenAsIdleBlocker(treatLockScreenAsIdleBox);
    QSignalBlocker applyActiveOnUnlockBlocker(applyActiveOnUnlockBox);
    QSignalBlocker applyActiveOnScreenOnBlocker(applyActiveOnScreenOnBox);
    QSignalBlocker pauseIdleWhileMediaPlayingBlocker(pauseIdleWhileMediaPlayingBox);
    QSignalBlocker debugLoggingBlocker(debugLoggingBox);
    QSignalBlocker idleTimeBlocker(idleTime);
    QSignalBlocker cooldownBlocker(cooldownSeconds);
    QSignalBlocker idleProfileBlocker(idleProfileBox);
    QSignalBlocker activeProfileBlocker(activeProfileBox);

    enabledBox->setChecked(plugin->GetEnabled());
    applyActiveOnStartBox->setChecked(plugin->GetApplyActiveOnStart());
    detectScreenOffBox->setChecked(plugin->GetDetectScreenOff());
    treatLockScreenAsIdleBox->setChecked(plugin->GetTreatLockScreenAsIdle());
    applyActiveOnUnlockBox->setChecked(plugin->GetApplyActiveOnUnlock());
    applyActiveOnScreenOnBox->setChecked(plugin->GetApplyActiveOnScreenOn());
    pauseIdleWhileMediaPlayingBox->setChecked(plugin->GetPauseIdleWhileMediaPlaying());
    debugLoggingBox->setChecked(plugin->GetDebugLogging());
    idleTime->setValue(plugin->GetIdleSeconds());
    cooldownSeconds->setValue(plugin->GetResumeCooldownSeconds());
    idleProfileBox->setEditText(plugin->GetIdleProfile());
    activeProfileBox->setEditText(plugin->GetActiveProfile());

    UpdateDependencies();
}

void IdleProfileWidget::UpdateDependencies()
{
    const bool detectScreenOff = detectScreenOffBox->isChecked();
    const bool treatLockScreenAsIdle = treatLockScreenAsIdleBox->isChecked();

    applyActiveOnScreenOnBox->setEnabled(detectScreenOff);
    applyActiveOnUnlockBox->setEnabled(treatLockScreenAsIdle);
}

void IdleProfileWidget::RefreshStatus()
{
    const bool isIdle = plugin->GetIsCurrentlyIdle();
    const qint64 lastSwitch = plugin->GetLastSwitchEpochMs();

    stateLabel->setText(isIdle ? "State: Idle" : "State: Active");

    if(lastSwitch <= 0)
    {
        lastSwitchLabel->setText("Last switch: Never");
    }
    else
    {
        const QString ts = QDateTime::fromMSecsSinceEpoch(lastSwitch).toString("yyyy-MM-dd hh:mm:ss");
        lastSwitchLabel->setText(QString("Last switch: %1").arg(ts));
    }
}

void IdleProfileWidget::Save()
{
    UpdateDependencies();

    const bool detectScreenOff = detectScreenOffBox->isChecked();
    const bool treatLockScreenAsIdle = treatLockScreenAsIdleBox->isChecked();
    const bool applyActiveOnScreenOn = detectScreenOff && applyActiveOnScreenOnBox->isChecked();
    const bool applyActiveOnUnlock = treatLockScreenAsIdle && applyActiveOnUnlockBox->isChecked();

    plugin->SetEnabled(enabledBox->isChecked());
    plugin->SetApplyActiveOnStart(applyActiveOnStartBox->isChecked());
    plugin->SetDetectScreenOff(detectScreenOff);
    plugin->SetTreatLockScreenAsIdle(treatLockScreenAsIdleBox->isChecked());
    plugin->SetApplyActiveOnUnlock(applyActiveOnUnlock);
    plugin->SetApplyActiveOnScreenOn(applyActiveOnScreenOn);
    plugin->SetPauseIdleWhileMediaPlaying(pauseIdleWhileMediaPlayingBox->isChecked());
    plugin->SetDebugLogging(debugLoggingBox->isChecked());
    plugin->SetIdleSeconds(idleTime->value());
    plugin->SetResumeCooldownSeconds(cooldownSeconds->value());
    plugin->SetIdleProfile(idleProfileBox->currentText());
    plugin->SetActiveProfile(activeProfileBox->currentText());
    plugin->SaveSettings();
}