#include <QApplication>
#include <QIcon>
#include "ClockWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Ustek");
    app.setApplicationName("plasma-clock-oled");
    app.setApplicationDisplayName("Plasma Clock OLED");
    app.setWindowIcon(QIcon(":/plasma-clock-oled.svg"));
    app.setQuitOnLastWindowClosed(false);  // Keep running, quit from tray

    ClockWidget clock;

    return app.exec();
}
