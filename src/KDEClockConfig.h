#pragma once

#include <QString>
#include <QRect>

// KDE Digital Clock config keys and defaults from:
// plasma-workspace/applets/digital-clock/package/contents/config/main.xml

struct KDEPanelConfig {
    int location = 4;      // 3=top, 4=bottom, 5=left, 6=right
    int thickness = 44;    // panel height/width
    bool floating = false;
    int screen = 0;

    static KDEPanelConfig load();
    QRect getPanelRect(const QRect& screenGeom) const;
};

struct KDEClockConfig {
    bool showDate = true;
    QString dateFormat = "shortDate";  // shortDate, longDate, isoDate, custom
    QString customDateFormat = "ddd d";
    int showSeconds = 1;  // 0=never, 1=tooltip only, 2=always
    int use24hFormat = 1; // 0=12h, 1=region default, 2=24h
    int dateDisplayFormat = 0;  // 0=adaptive, 1=beside, 2=below

    static KDEClockConfig load();
};
