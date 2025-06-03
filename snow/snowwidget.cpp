#include "snowwidget.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <cmath> // for std::sin

SnowWidget::SnowWidget(QWidget *parent)
    : QWidget(parent), m_animationTimer(new QTimer(this)), m_rng(std::random_device{}())
{
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint |
                   Qt::Tool |
                   Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_screenBoundingRect = getCombinedScreenGeometry();
    setGeometry(m_screenBoundingRect);
    qDebug() << "Combined screen geometry:" << m_screenBoundingRect;

    m_unicodeSnowChars << "❄"  // 标准雪花 (U+2744)
                       << "❅"  // 紧凑三瓣雪花 (U+2745)
                       << "❆"  // 重瓣雪花 (U+2746)
                       << "·" // 中点 (U+00B7)
                       << "."; // 句号 (U+002E)


    for (int i = 0; i < 25; ++i) {
        m_snowSizes.append(8 + i * 2);
    }
    if (m_snowSizes.size() < 20) {
        int baseSize = m_snowSizes.isEmpty() ? 8 : m_snowSizes.last() + 2;
        for (int i = 0; i < 20 - m_snowSizes.size(); ++i) {
            m_snowSizes.append(baseSize + i * 2);
        }
    }

    m_numSnowflakes = 600;
    initializeSnowflakes();

    connect(m_animationTimer, &QTimer::timeout, this, &SnowWidget::updateSnowflakes);
    m_animationTimer->start(30);

    setupTrayIcon();
}

SnowWidget::~SnowWidget()
{
}

QRect SnowWidget::getCombinedScreenGeometry()
{
    QRect combinedGeometry;
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return QGuiApplication::primaryScreen()->geometry();
    }
    for (QScreen *screen : std::as_const(screens)) {
        combinedGeometry = combinedGeometry.united(screen->geometry());
    }
    return combinedGeometry;
}

void SnowWidget::initializeSnowflakes()
{
    m_snowflakes.clear();
    m_snowflakes.reserve(m_numSnowflakes);

    std::uniform_real_distribution<double> xDist(m_screenBoundingRect.left(), m_screenBoundingRect.right());

    // *** MODIFICATION HERE ***
    // Ensure snowflakes start above the screen.
    // Distribute them over a range, e.g., from one screen height above down to just above the top.
    double startYMin = m_screenBoundingRect.top() - m_screenBoundingRect.height(); // Start up to one screen height above
    double startYMax = m_screenBoundingRect.top() - 10; // Ensure they are at least a bit above the top edge
    // Handle cases where screen height might be very small or initial geometry not fully set
    if (startYMin >= startYMax) {
        startYMin = m_screenBoundingRect.top() - 500; // Fallback: start up to 500px above
        startYMax = m_screenBoundingRect.top() - 10;
        if (startYMin >= startYMax) { // If top is near 0
            startYMin = -500;
            startYMax = -10;
        }
    }
    std::uniform_real_distribution<double> yDist(startYMin, startYMax);
    // *** END MODIFICATION ***

    std::uniform_real_distribution<double> speedYDist(0.5, 1.5);
    std::uniform_real_distribution<double> speedXBaseDist(-0.15, 0.15);

    std::uniform_int_distribution<int> charIndexDist(0, m_unicodeSnowChars.size() - 1);
    std::uniform_int_distribution<int> sizeIndexDist(0, m_snowSizes.size() - 1);
    std::uniform_real_distribution<double> opacityDist(0.5, 1.0);
    std::uniform_real_distribution<double> initialXOffsetDist(0, 2 * M_PI);

    std::uniform_real_distribution<double> swayFactorDist(1.0, 4.0);
    std::uniform_real_distribution<double> swaySpeedDist(0.001, 0.0025);


    std::uniform_int_distribution<int> hueDist(0, 359); // 色相: 0-359
    std::uniform_int_distribution<int> satDist(180, 255); // 饱和度: 鲜艳一些
    std::uniform_int_distribution<int> valDist(200, 255); // 亮度: 明亮一些

    for (int i = 0; i < m_numSnowflakes; ++i) {
        Snowflake s;
        s.position = QPointF(xDist(m_rng), yDist(m_rng)); // yDist now ensures they start above
        s.speedY = speedYDist(m_rng);
        s.character = m_unicodeSnowChars[charIndexDist(m_rng)];
        s.size = m_snowSizes[sizeIndexDist(m_rng)];
        s.opacity = opacityDist(m_rng);
        s.initialXOffset = initialXOffsetDist(m_rng);
        s.swayFactor = swayFactorDist(m_rng);
        s.swaySpeed = swaySpeedDist(m_rng);
        s.speedX = speedXBaseDist(m_rng);
        // s.color = QColor::fromHsv(hueDist(m_rng), satDist(m_rng), valDist(m_rng));
        s.color = Qt::white;
        m_snowflakes.append(s);
    }
}

void SnowWidget::updateSnowflakes()
{
    double timeElapsed = m_animationTimer->interval() / 1000.0;

    for (Snowflake &s : m_snowflakes) {
        s.position.ry() += s.speedY * (s.size / 15.0);

        double sway = s.swayFactor * std::sin(s.initialXOffset + s.position.y() * s.swaySpeed);
        s.position.rx() += s.speedX + sway * timeElapsed * 15;
        s.initialXOffset += 0.0005;


        if (s.position.y() - s.size > m_screenBoundingRect.bottom()) {
            s.position.setY(m_screenBoundingRect.top() - s.size - std::uniform_real_distribution<double>(0, 50)(m_rng));
            s.position.setX(std::uniform_real_distribution<double>(m_screenBoundingRect.left(), m_screenBoundingRect.right())(m_rng));
            s.speedY = std::uniform_real_distribution<double>(0.5, 1.5)(m_rng);
            s.character = m_unicodeSnowChars[std::uniform_int_distribution<int>(0, m_unicodeSnowChars.size() - 1)(m_rng)];
            s.size = m_snowSizes[std::uniform_int_distribution<int>(0, m_snowSizes.size() - 1)(m_rng)];
            s.opacity = std::uniform_real_distribution<double>(0.5, 1.0)(m_rng);

            s.swayFactor = std::uniform_real_distribution<double>(1.0, 4.0)(m_rng);
            s.swaySpeed = std::uniform_real_distribution<double>(0.001, 0.0025)(m_rng);
            s.speedX = std::uniform_real_distribution<double>(-0.15, 0.15)(m_rng);
        }

        if (s.position.x() + s.size < m_screenBoundingRect.left()) {
            s.position.setX(m_screenBoundingRect.right() + s.size / 2.0);
        } else if (s.position.x() - s.size > m_screenBoundingRect.right()) {
            s.position.setX(m_screenBoundingRect.left() - s.size / 2.0);
        }
    }
    update();
}

void SnowWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (const Snowflake &s : m_snowflakes) {
        QFont font("Arial", s.size);
        painter.setFont(font);
        QColor snowColor = s.color;
        snowColor.setAlphaF(s.opacity);
        painter.setPen(snowColor);
        painter.drawText(s.position, s.character);
    }
}

void SnowWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void SnowWidget::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray not available, cannot create tray icon.";
        // Fallback: ensure Esc key works or provide another exit method
        setFocusPolicy(Qt::StrongFocus); // Try to make Esc key work
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this); // Parented to SnowWidget

    QIcon icon(":/icon/snowflake.ico");
    if (icon.isNull()) {
        QPixmap fallbackPixmap(16,16);
        fallbackPixmap.fill(Qt::white);
        QPainter painter(&fallbackPixmap);
        painter.setPen(Qt::blue);
        painter.drawText(fallbackPixmap.rect(), Qt::AlignCenter, "S");
        icon = QIcon(fallbackPixmap);
    }
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("Snow \nRight-click to quit");

    m_trayMenu = new QMenu(this); // Parented to SnowWidget
    QAction *quitAction = m_trayMenu->addAction("Quit Snow");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    // Optional: Handle tray icon activation (e.g., left click)
    // connect(m_trayIcon, &QSystemTrayIcon::activated, this, [&](QSystemTrayIcon::ActivationReason reason){
    //     if (reason == QSystemTrayIcon::Trigger) { // Typically left click
    //         // Maybe show a settings dialog or hide/show snow
    //     }
    // });
}
