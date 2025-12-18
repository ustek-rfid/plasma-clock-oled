#include <QApplication>
#include <QIcon>
#include <QLockFile>
#include <QStandardPaths>
#include <QDir>
#include "ClockWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Ustek");
    app.setApplicationName("plasma-clock-oled");
    app.setApplicationDisplayName("Plasma Clock OLED");
    app.setWindowIcon(QIcon(":/plasma-clock-oled.svg"));
    app.setQuitOnLastWindowClosed(false);  // Keep running, quit from tray

    // Ensure only one instance runs
    QString lockPath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (lockPath.isEmpty())
        lockPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QLockFile lockFile(lockPath + QDir::separator() + "plasma-clock-oled.lock");

    if (!lockFile.tryLock(100)) {
        qWarning("Another instance is already running.");
        return 1;
    }

    ClockWidget clock;

    return app.exec();
}
