#include "KDEClockConfig.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>

// Helper to read a value from config section
static QString readValue(const QString& section, const QString& key, const QString& defaultVal)
{
    QRegularExpression rx(key + R"(=(.+))");
    QRegularExpressionMatch m = rx.match(section);
    if (m.hasMatch()) {
        return m.captured(1).trimmed();
    }
    return defaultVal;
}

static int readInt(const QString& section, const QString& key, int defaultVal)
{
    QString val = readValue(section, key, QString::number(defaultVal));
    bool ok;
    int result = val.toInt(&ok);
    return ok ? result : defaultVal;
}

static bool readBool(const QString& section, const QString& key, bool defaultVal)
{
    QString val = readValue(section, key, defaultVal ? "true" : "false");
    return val == "true" || val == "1";
}

KDEPanelConfig KDEPanelConfig::load()
{
    KDEPanelConfig config;

    // Read panel location from appletsrc
    QString appletsPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                          + "/plasma-org.kde.plasma.desktop-appletsrc";
    QFile appletsFile(appletsPath);
    if (appletsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QTextStream(&appletsFile).readAll();
        appletsFile.close();

        // Find panel containment with digitalclock
        QRegularExpression panelRx(R"(\[Containments\]\[(\d+)\][^\[]*plugin=org\.kde\.panel)");
        QRegularExpressionMatchIterator it = panelRx.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString panelId = match.captured(1);

            // Check if this panel has the digitalclock
            if (content.contains(QString("[Containments][%1][Applets]").arg(panelId)) &&
                content.contains("plugin=org.kde.plasma.digitalclock")) {

                // Get panel section
                QRegularExpression sectionRx(R"(\[Containments\]\[)" + panelId + R"(\]([^\[]*))");
                QRegularExpressionMatch sectionMatch = sectionRx.match(content);
                if (sectionMatch.hasMatch()) {
                    QString section = sectionMatch.captured(1);
                    config.location = readInt(section, "location", 4);
                    config.screen = readInt(section, "lastScreen", 0);
                }
                break;
            }
        }
    }

    // Read panel thickness from plasmashellrc
    QString shellPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                        + "/plasmashellrc";
    QFile shellFile(shellPath);
    if (shellFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QTextStream(&shellFile).readAll();
        shellFile.close();

        // Find Panel 2 Defaults section (common panel)
        QRegularExpression defaultsRx(R"(\[PlasmaViews\]\[Panel \d+\]\[Defaults\]([^\[]*))");
        QRegularExpressionMatch defaultsMatch = defaultsRx.match(content);
        if (defaultsMatch.hasMatch()) {
            QString section = defaultsMatch.captured(1);
            config.thickness = readInt(section, "thickness", 44);
        }

        // Check floating
        QRegularExpression panelRx(R"(\[PlasmaViews\]\[Panel \d+\]([^\[]*\[Defaults\]))");
        QRegularExpressionMatch panelMatch = panelRx.match(content);
        if (panelMatch.hasMatch()) {
            QString section = panelMatch.captured(1);
            config.floating = readBool(section, "floating", false);
        }
    }

    return config;
}

QRect KDEPanelConfig::getPanelRect(const QRect& screenGeom) const
{
    switch (location) {
        case 3: // Top
            return QRect(screenGeom.x(), screenGeom.y(),
                        screenGeom.width(), thickness);
        case 4: // Bottom
            return QRect(screenGeom.x(), screenGeom.y() + screenGeom.height() - thickness,
                        screenGeom.width(), thickness);
        case 5: // Left
            return QRect(screenGeom.x(), screenGeom.y(),
                        thickness, screenGeom.height());
        case 6: // Right
            return QRect(screenGeom.x() + screenGeom.width() - thickness, screenGeom.y(),
                        thickness, screenGeom.height());
        default:
            return QRect(screenGeom.x(), screenGeom.y() + screenGeom.height() - thickness,
                        screenGeom.width(), thickness);
    }
}

KDEClockConfig KDEClockConfig::load()
{
    KDEClockConfig config;

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                         + "/plasma-org.kde.plasma.desktop-appletsrc";

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open plasma config, using defaults";
        return config;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // Find the digitalclock applet section
    QRegularExpression appletRx(R"(\[Containments\]\[\d+\]\[Applets\]\[(\d+)\][^\[]*plugin=org\.kde\.plasma\.digitalclock)");
    QRegularExpressionMatch appletMatch = appletRx.match(content);

    if (!appletMatch.hasMatch()) {
        qDebug() << "Digital clock applet not found, using defaults";
        return config;
    }

    QString appletId = appletMatch.captured(1);

    // Find the Appearance section for this applet
    QRegularExpression sectionRx(
        R"(\[Containments\]\[\d+\]\[Applets\]\[)" + appletId + R"(\]\[Configuration\]\[Appearance\]([^\[]*))");
    QRegularExpressionMatch sectionMatch = sectionRx.match(content);

    if (!sectionMatch.hasMatch()) {
        qDebug() << "Appearance section not found, using defaults";
        return config;
    }

    QString appearanceSection = sectionMatch.captured(1);

    config.showDate = readBool(appearanceSection, "showDate", true);
    config.dateFormat = readValue(appearanceSection, "dateFormat", "shortDate");
    config.customDateFormat = readValue(appearanceSection, "customDateFormat", "ddd d");
    config.showSeconds = readInt(appearanceSection, "showSeconds", 1);
    config.use24hFormat = readInt(appearanceSection, "use24hFormat", 1);
    config.dateDisplayFormat = readInt(appearanceSection, "dateDisplayFormat", 0);

    return config;
}
