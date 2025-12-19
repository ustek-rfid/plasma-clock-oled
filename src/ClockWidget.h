#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QScreen>
#include <QFileSystemWatcher>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QSettings>
#include <random>
#include "KDEClockConfig.h"

class ClockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClockWidget(QWidget *parent = nullptr);
    ~ClockWidget() override = default;

signals:
    void recreationRequested();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void updateTime();
    void repositionClock();
    void onConfigFileChanged(const QString& path);
    void toggleTrayIcon();
    void onScreenAdded(QScreen* screen);

private:
    void setupWindow();
    void setupAppearance();
    void setupTimers();
    void setupConfigWatcher();
    void setupTrayIcon();
    QIcon createTrayIcon();
    void configureLayerShell();
    void calculateBounds();
    void reloadConfig();
    void loadSettings();
    void saveSettings();
    int randomPosition();
    QString formatTime(const QTime& time);
    QString formatDate(const QDate& date);
    bool isVerticalPanel() const;

    QLabel* m_timeLabel;
    QLabel* m_dateLabel;
    QTimer* m_clockTimer;
    QTimer* m_repositionTimer;
    QFileSystemWatcher* m_configWatcher;
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_contextMenu;
    QAction* m_toggleTrayAction;
    QSettings m_settings;

    int m_minPos;
    int m_maxPos;
    bool m_showTrayIcon;

    std::mt19937 m_rng;
    KDEClockConfig m_kdeConfig;
    KDEPanelConfig m_panelConfig;
    QRect m_panelRect;
};
