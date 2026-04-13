#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QShowEvent>
#include <QLabel>
#include <QTimer>

#include "IdleProfilePlugin.h"

class IdleProfileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IdleProfileWidget(IdleProfilePlugin* plugin, QWidget* parent = nullptr);

private:
    IdleProfilePlugin* plugin;

    QSpinBox* idleTime;
    QComboBox* idleProfileBox;
    QComboBox* activeProfileBox;
    QCheckBox* enabledBox;
    QCheckBox* applyActiveOnStartBox;
    QCheckBox* detectScreenOffBox;
    QCheckBox* applyActiveOnScreenOnBox;
    QCheckBox* debugLoggingBox;
    QSpinBox* cooldownSeconds;
    QPushButton* refreshProfilesButton;
    QLabel* stateLabel;
    QLabel* lastSwitchLabel;
    QTimer* statusTimer;

    void showEvent(QShowEvent* event) override;
    void RefreshProfiles();
    void RefreshStatus();
    void RefreshUI();
    void UpdateScreenStateDependency();
    void Save();
};