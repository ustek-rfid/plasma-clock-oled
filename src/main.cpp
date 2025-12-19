#include <QApplication>
#include <QIcon>
#include <QLockFile>
#include <QStandardPaths>
#include <QDir>
#include <QPointer>
#include <QDebug>
#include "ClockWidget.h"

static QPointer<ClockWidget> g_clock;

static void createClock()
{
    qDebug() << "Creating new ClockWidget";
    g_clock = new ClockWidget();
    QObject::connect(g_clock, &ClockWidget::recreationRequested, []() {
        qDebug() << "Recreation requested, scheduling...";
        // Delete old and create new on next event loop iteration
        if (g_clock) {
            g_clock->deleteLater();
        }
        QMetaObject::invokeMethod(qApp, []() {
            createClock();
        }, Qt::QueuedConnection);
    });
}

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

    createClock();

    return app.exec();
}
