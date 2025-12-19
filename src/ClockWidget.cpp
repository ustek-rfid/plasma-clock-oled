#include "ClockWidget.h"
#include "Config.h"

#include <QApplication>
#include <QGuiApplication>
#include <QDateTime>
#include <QLocale>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QStandardPaths>
#include <QDebug>
#include <QIcon>
#include <QContextMenuEvent>
#include <QAction>
#include <QCursor>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

#include <LayerShellQt/Window>
#include <LayerShellQt/Shell>

ClockWidget::ClockWidget(QWidget *parent)
    : QWidget(parent)
    , m_timeLabel(nullptr)
    , m_dateLabel(nullptr)
    , m_clockTimer(nullptr)
    , m_repositionTimer(nullptr)
    , m_configWatcher(nullptr)
    , m_trayIcon(nullptr)
    , m_contextMenu(nullptr)
    , m_toggleTrayAction(nullptr)
    , m_settings("Ustek", "plasma-clock-oled")
    , m_minPos(0)
    , m_maxPos(0)
    , m_showTrayIcon(true)
    , m_rng(std::random_device{}())
    , m_kdeConfig(KDEClockConfig::load())
    , m_panelConfig(KDEPanelConfig::load())
{
    loadSettings();
    setupAppearance();
    setupWindow();
    updateTime();
    calculateBounds();

    // Configure layer shell before showing
    configureLayerShell();

    show();
    setupTimers();
    setupConfigWatcher();
    setupTrayIcon();

    // Watch for screen changes (handles lock/unlock where outputs are removed/added)
    connect(qApp, &QGuiApplication::screenAdded,
            this, &ClockWidget::onScreenAdded);
}

void ClockWidget::configureLayerShell()
{
    // Create window handle
    winId();

    if (auto* layerWindow = LayerShellQt::Window::get(windowHandle())) {
        layerWindow->setLayer(LayerShellQt::Window::LayerBottom);
        layerWindow->setExclusiveZone(0);
        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);

        // Set anchors based on panel location
        LayerShellQt::Window::Anchors anchors;
        switch (m_panelConfig.location) {
            case 3: // Top
                anchors = LayerShellQt::Window::Anchors(
                    LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft);
                break;
            case 4: // Bottom
            default:
                anchors = LayerShellQt::Window::Anchors(
                    LayerShellQt::Window::AnchorBottom | LayerShellQt::Window::AnchorLeft);
                break;
            case 5: // Left
                anchors = LayerShellQt::Window::Anchors(
                    LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorTop);
                break;
            case 6: // Right
                anchors = LayerShellQt::Window::Anchors(
                    LayerShellQt::Window::AnchorRight | LayerShellQt::Window::AnchorTop);
                break;
        }
        layerWindow->setAnchors(anchors);

        repositionClock();
    }
}

void ClockWidget::setupWindow()
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setStyleSheet("background: transparent;");
}

void ClockWidget::setupAppearance()
{
    QString style = QString("color: %1; background-color: transparent;").arg(Config::FontColor);

    const int panelThickness = m_panelConfig.thickness;
    const bool vertical = isVerticalPanel();

    // Determine the sample text for width calculation
    QString timeSample = (m_kdeConfig.showSeconds == 2) ? "00:00:00" : "00:00";
    if (m_kdeConfig.use24hFormat == 0) {
        timeSample = (m_kdeConfig.showSeconds == 2) ? "00:00:00 AM" : "00:00 AM";
    }

    // Determine date sample based on format
    QString dateSample;
    if (m_kdeConfig.dateFormat == "longDate") {
        dateSample = "Wednesday, December 30";  // Approximate longest
    } else if (m_kdeConfig.dateFormat == "isoDate") {
        dateSample = "0000-00-00";
    } else {
        dateSample = "00.00.0000";  // Short date
    }

    int timeFontSize, dateFontSize = 0;
    int timeCapHeight = 0, dateCapHeight = 0;

    if (vertical) {
        // For vertical panels: each label fills width independently
        const int availableWidth = panelThickness - 4;  // 2px padding each side

        // Find time font size that fills width
        double timeRatio = 1.0;
        for (int attempt = 0; attempt < 20; attempt++) {
            timeFontSize = static_cast<int>(panelThickness * timeRatio);
            QFont timeFont(Config::FontFamily);
            timeFont.setPixelSize(timeFontSize);
            QFontMetrics timeFm(timeFont);
            if (timeFm.horizontalAdvance(timeSample) <= availableWidth) {
                timeCapHeight = timeFm.capHeight();
                break;
            }
            timeRatio *= 0.9;
        }

        // Find date font size that fills width independently
        if (m_kdeConfig.showDate) {
            double dateRatio = 1.0;
            for (int attempt = 0; attempt < 20; attempt++) {
                dateFontSize = static_cast<int>(panelThickness * dateRatio);
                QFont dateFont(Config::FontFamily);
                dateFont.setPixelSize(dateFontSize);
                QFontMetrics dateFm(dateFont);
                if (dateFm.horizontalAdvance(dateSample) <= availableWidth) {
                    dateCapHeight = dateFm.capHeight();
                    break;
                }
                dateRatio *= 0.9;
            }
        }
    } else {
        // For horizontal panels: use KDE Digital Clock ratios
        // time 0.56, date 0.8 * time
        double timeRatio = 0.56;
        double dateRatio = 0.8;

        for (int attempt = 0; attempt < 20; attempt++) {
            if (m_kdeConfig.showDate) {
                timeFontSize = static_cast<int>(panelThickness * timeRatio);
                dateFontSize = static_cast<int>(timeFontSize * dateRatio);
            } else {
                timeFontSize = static_cast<int>(panelThickness * 0.71);
            }

            QFont timeFont(Config::FontFamily);
            timeFont.setPixelSize(timeFontSize);
            QFontMetrics timeFm(timeFont);
            timeCapHeight = timeFm.capHeight();

            int totalHeight = timeCapHeight + 2;
            if (m_kdeConfig.showDate) {
                QFont dateFont(Config::FontFamily);
                dateFont.setPixelSize(dateFontSize);
                QFontMetrics dateFm(dateFont);
                dateCapHeight = dateFm.capHeight();
                totalHeight += 4 + dateCapHeight + 2;
            }

            if (totalHeight <= panelThickness) {
                break;
            }
            timeRatio *= 0.9;
        }
    }

    // Clamp to reasonable range
    if (timeFontSize < 8) timeFontSize = 8;
    if (dateFontSize < 8 && m_kdeConfig.showDate) dateFontSize = 8;

    QFont timeFont(Config::FontFamily);
    timeFont.setPixelSize(timeFontSize);
    timeFont.setWeight(QFont::Normal);
    QFontMetrics timeFm(timeFont);

    qDebug() << "Time font:" << timeFontSize << "Date font:" << dateFontSize
             << "panel:" << panelThickness << (vertical ? "(vertical)" : "(horizontal)");

    // Calculate total content height and widget width
    // Add 2px buffer for time, and 2px buffer for date if shown
    int totalHeight = timeCapHeight + 2;
    if (m_kdeConfig.showDate) {
        totalHeight += 4 + dateCapHeight + 2;
    }

    // Create time label
    m_timeLabel = new QLabel(this);
    m_timeLabel->setFont(timeFont);
    m_timeLabel->setStyleSheet(style);
    m_timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_timeLabel->setText("00:00");
    m_timeLabel->adjustSize();

    // How much space is above capHeight (for accents we don't have on numbers)
    // Keep 2px buffer to avoid clipping top of numbers
    int timeTopPadding = timeFm.ascent() - timeCapHeight - 2;
    if (timeTopPadding < 0) timeTopPadding = 0;

    // Position time label so capHeight portion starts at y=0 (let top padding get clipped)
    m_timeLabel->move(0, -timeTopPadding);

    int widgetWidth = m_timeLabel->width();

    if (m_kdeConfig.showDate) {
        QFont dateFont(Config::FontFamily);
        dateFont.setPixelSize(dateFontSize);
        dateFont.setWeight(QFont::Normal);
        QFontMetrics dateFm(dateFont);

        m_dateLabel = new QLabel(this);
        m_dateLabel->setFont(dateFont);
        m_dateLabel->setStyleSheet(style);
        m_dateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        m_dateLabel->setText("00.00.0000");
        m_dateLabel->adjustSize();

        // Keep 2px buffer for date label as well
        int dateTopPadding = dateFm.ascent() - dateCapHeight - 2;
        if (dateTopPadding < 0) dateTopPadding = 0;

        // Position: capHeight of time ends at timeCapHeight, add 4px gap
        // Then offset date label up by its top padding so capHeight starts at the gap
        int dateLabelY = timeCapHeight + 4 - dateTopPadding + 2;  // +2 to account for time buffer
        m_dateLabel->move(0, dateLabelY);

        widgetWidth = qMax(widgetWidth, m_dateLabel->width());
    }

    // Center labels horizontally
    m_timeLabel->move((widgetWidth - m_timeLabel->width()) / 2, m_timeLabel->y());
    if (m_dateLabel) {
        m_dateLabel->move((widgetWidth - m_dateLabel->width()) / 2, m_dateLabel->y());
    }

    // Widget height is exactly the calculated totalHeight (capHeight based)
    resize(widgetWidth, totalHeight);

    // Ensure labels are visible
    m_timeLabel->show();
    if (m_dateLabel) {
        m_dateLabel->show();
    }
}

void ClockWidget::setupTimers()
{
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &ClockWidget::updateTime);
    m_clockTimer->start(Config::ClockUpdateIntervalMs);

    m_repositionTimer = new QTimer(this);
    connect(m_repositionTimer, &QTimer::timeout, this, &ClockWidget::repositionClock);
    m_repositionTimer->start(Config::RepositionIntervalMs);

    connect(QGuiApplication::primaryScreen(), &QScreen::geometryChanged,
            this, &ClockWidget::calculateBounds);
    connect(QGuiApplication::primaryScreen(), &QScreen::geometryChanged,
            this, &ClockWidget::repositionClock);
}

bool ClockWidget::isVerticalPanel() const
{
    return m_panelConfig.location == 5 || m_panelConfig.location == 6;
}

void ClockWidget::calculateBounds()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeom = screen->geometry();

    // Get panel rectangle
    m_panelRect = m_panelConfig.getPanelRect(screenGeom);

    // Calculate bounds within the panel
    // For horizontal panels (top/bottom), move along X axis
    // For vertical panels (left/right), move along Y axis
    if (isVerticalPanel()) {
        // Vertical panel - clock moves up/down within panel
        m_minPos = Config::HorizontalPadding;
        m_maxPos = m_panelRect.height() - height() - Config::HorizontalPadding;
    } else {
        // Horizontal panel - clock moves left/right within panel
        m_minPos = Config::HorizontalPadding;
        m_maxPos = m_panelRect.width() - width() - Config::HorizontalPadding;
    }
}

void ClockWidget::repositionClock()
{
    if (auto* layerWindow = LayerShellQt::Window::get(windowHandle())) {
        int pos = randomPosition();

        QMargins margins;
        if (isVerticalPanel()) {
            // Vertical panel - center horizontally, random vertical position
            int widgetWidth = width();
            int hOffset = (m_panelConfig.thickness - widgetWidth) / 2;
            if (hOffset < 0) hOffset = 0;

            qDebug() << "Panel thickness:" << m_panelConfig.thickness
                     << "Widget width:" << widgetWidth
                     << "hOffset:" << hOffset;

            if (m_panelConfig.location == 5) { // Left panel
                margins = QMargins(hOffset, pos, 0, 0);
            } else { // Right panel
                margins = QMargins(0, pos, hOffset, 0);
            }
        } else {
            // Horizontal panel - center vertically, random horizontal position
            int widgetHeight = height();
            int vOffset = (m_panelConfig.thickness - widgetHeight) / 2;
            if (vOffset < 0) vOffset = 0;

            qDebug() << "Panel thickness:" << m_panelConfig.thickness
                     << "Widget height:" << widgetHeight
                     << "vOffset:" << vOffset;

            if (m_panelConfig.location == 3) { // Top panel
                margins = QMargins(pos, vOffset, 0, 0);
            } else { // Bottom panel (default)
                margins = QMargins(pos, 0, 0, vOffset);
            }
        }
        layerWindow->setMargins(margins);
    }
}

int ClockWidget::randomPosition()
{
    if (m_maxPos <= m_minPos) {
        return m_minPos;
    }
    std::uniform_int_distribution<int> dist(m_minPos, m_maxPos);
    return dist(m_rng);
}

QString ClockWidget::formatTime(const QTime& time)
{
    QLocale locale = QLocale::system();

    // Handle 24h format setting
    // 0 = 12h, 1 = region default, 2 = 24h
    QString timeStr;
    if (m_kdeConfig.use24hFormat == 0) {
        timeStr = time.toString("h:mm AP");
    } else if (m_kdeConfig.use24hFormat == 2) {
        timeStr = time.toString("HH:mm");
    } else {
        // Region default
        timeStr = locale.toString(time, QLocale::ShortFormat);
    }

    // Handle seconds display
    // 0 = never, 1 = tooltip only, 2 = always
    if (m_kdeConfig.showSeconds == 2) {
        if (m_kdeConfig.use24hFormat == 0) {
            timeStr = time.toString("h:mm:ss AP");
        } else if (m_kdeConfig.use24hFormat == 2) {
            timeStr = time.toString("HH:mm:ss");
        } else {
            // Region default with seconds
            QString fmt = locale.timeFormat(QLocale::ShortFormat);
            if (!fmt.contains("ss")) {
                fmt.replace("mm", "mm:ss");
            }
            timeStr = locale.toString(time, fmt);
        }
    }

    return timeStr;
}

QString ClockWidget::formatDate(const QDate& date)
{
    QLocale locale = QLocale::system();

    if (m_kdeConfig.dateFormat == "shortDate") {
        return locale.toString(date, QLocale::ShortFormat);
    } else if (m_kdeConfig.dateFormat == "longDate") {
        return locale.toString(date, QLocale::LongFormat);
    } else if (m_kdeConfig.dateFormat == "isoDate") {
        return date.toString(Qt::ISODate);
    } else if (m_kdeConfig.dateFormat == "custom") {
        return locale.toString(date, m_kdeConfig.customDateFormat);
    }

    return locale.toString(date, QLocale::ShortFormat);
}

void ClockWidget::updateTime()
{
    QDateTime now = QDateTime::currentDateTime();

    m_timeLabel->setText(formatTime(now.time()));

    if (m_dateLabel && m_kdeConfig.showDate) {
        m_dateLabel->setText(formatDate(now.date()));
    }
}

void ClockWidget::setupConfigWatcher()
{
    m_configWatcher = new QFileSystemWatcher(this);

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString appletsrc = configDir + "/plasma-org.kde.plasma.desktop-appletsrc";
    QString shellrc = configDir + "/plasmashellrc";

    m_configWatcher->addPath(appletsrc);
    m_configWatcher->addPath(shellrc);

    connect(m_configWatcher, &QFileSystemWatcher::fileChanged,
            this, &ClockWidget::onConfigFileChanged);

    qDebug() << "Watching config files:" << appletsrc << shellrc;
}

void ClockWidget::onConfigFileChanged(const QString& path)
{
    qDebug() << "Config file changed:" << path;

    // Re-add the path (QFileSystemWatcher removes it after change on some systems)
    if (!m_configWatcher->files().contains(path)) {
        m_configWatcher->addPath(path);
    }

    // Debounce: use a single-shot timer to avoid multiple reloads
    QTimer::singleShot(500, this, &ClockWidget::reloadConfig);
}

void ClockWidget::reloadConfig()
{
    qDebug() << "Reloading panel configuration...";

    KDEPanelConfig newPanelConfig = KDEPanelConfig::load();

    // Check if panel changed
    if (newPanelConfig.thickness != m_panelConfig.thickness ||
        newPanelConfig.location != m_panelConfig.location) {

        qDebug() << "Panel config changed - thickness:" << newPanelConfig.thickness
                 << "location:" << newPanelConfig.location;

        m_panelConfig = newPanelConfig;

        // Delete old labels
        delete m_timeLabel;
        m_timeLabel = nullptr;
        delete m_dateLabel;
        m_dateLabel = nullptr;

        // Clear any size constraints before rebuilding
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        resize(0, 0);  // Reset size to force recalculation

        // Rebuild
        setupAppearance();
        updateTime();

        qDebug() << "After rebuild - Widget size:" << size();

        calculateBounds();
        configureLayerShell();
    }
}

QIcon ClockWidget::createTrayIcon()
{
    // Determine icon color based on system theme
    QPalette palette = QApplication::palette();
    QColor windowColor = palette.color(QPalette::Window);
    bool isDark = windowColor.lightness() < 128;
    QString color = isDark ? "#eeeeee" : "#232629";

    // Load SVG and replace color
    QFile file(":/plasma-clock-oled.svg");
    if (!file.open(QIODevice::ReadOnly)) {
        return QIcon::fromTheme("clock", QIcon::fromTheme("preferences-system-time"));
    }

    QString svgContent = QString::fromUtf8(file.readAll());
    file.close();

    // Replace currentColor with theme-appropriate color
    svgContent.replace("currentColor", color);

    // Create icon from modified SVG
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QSvgRenderer renderer(svgContent.toUtf8());
    QPainter painter(&pixmap);
    renderer.render(&painter);

    return QIcon(pixmap);
}

void ClockWidget::setupTrayIcon()
{
    // Create context menu (shared between clock widget and tray)
    // Use nullptr parent to avoid inheriting transparent background
    m_contextMenu = new QMenu();
    m_contextMenu->setAttribute(Qt::WA_TranslucentBackground, false);
    m_toggleTrayAction = m_contextMenu->addAction(
        m_showTrayIcon ? "Hide Tray Icon" : "Show Tray Icon",
        this, &ClockWidget::toggleTrayIcon);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction("Quit", qApp, &QApplication::quit);

    // Setup tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(createTrayIcon());
    m_trayIcon->setToolTip("Plasma Clock OLED");
    m_trayIcon->setContextMenu(m_contextMenu);

    if (m_showTrayIcon) {
        m_trayIcon->show();
    }
}

void ClockWidget::contextMenuEvent(QContextMenuEvent* event)
{
    Q_UNUSED(event);
    if (m_contextMenu) {
        // Use cursor position - more reliable on Wayland
        m_contextMenu->popup(QCursor::pos());
    }
}

void ClockWidget::toggleTrayIcon()
{
    m_showTrayIcon = !m_showTrayIcon;

    if (m_showTrayIcon) {
        m_trayIcon->show();
        m_toggleTrayAction->setText("Hide Tray Icon");
    } else {
        m_trayIcon->hide();
        m_toggleTrayAction->setText("Show Tray Icon");
    }

    saveSettings();
}

void ClockWidget::loadSettings()
{
    m_showTrayIcon = m_settings.value("showTrayIcon", true).toBool();
}

void ClockWidget::saveSettings()
{
    m_settings.setValue("showTrayIcon", m_showTrayIcon);
    m_settings.sync();
}

void ClockWidget::onScreenAdded(QScreen* screen)
{
    // Ignore placeholder screens (empty name means no real output)
    if (screen->name().isEmpty()) {
        qDebug() << "Ignoring placeholder screen";
        return;
    }

    qDebug() << "Real screen added:" << screen->name();

    // Request widget recreation to get a fresh layer shell surface
    // The old widget can't be converted back to layer shell after screen changes
    QTimer::singleShot(200, this, [this]() {
        qDebug() << "Requesting widget recreation";
        emit recreationRequested();
    });
}
