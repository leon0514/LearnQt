#include "overlaywidget.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGuiApplication> // For QScreen
#include <QDebug>
#include <QKeyEvent>

OverlayWidget::OverlayWidget(QScreen *screen, QWidget *parent)
    : QWidget(parent), targetScreen(screen)
{
    if (!targetScreen) {
        qWarning() << "OverlayWidget created with null screen!";
        targetScreen = QGuiApplication::primaryScreen(); // Fallback
    }

    setWindowFlags(Qt::FramelessWindowHint |       // No window decorations
                   Qt::WindowStaysOnTopHint |     // Always on top
                   Qt::Tool);                     // Doesn't appear in taskbar, less obtrusive

    setAttribute(Qt::WA_TranslucentBackground);    // Allows painting with alpha
    // setAttribute(Qt::WA_DeleteOnClose);         // Ensure deleted when closed, handled by SettingsDialog now

    // Set geometry for the specific screen
    if (targetScreen) {
        this->setGeometry(targetScreen->geometry());
    } else { // Fallback if screen somehow null
        this->setGeometry(QGuiApplication::primaryScreen()->geometry());
    }

    skipButton = new QPushButton("提前结束", this);
    skipButton->setMinimumHeight(40);
    skipButton->setMinimumWidth(150);
    skipButton->setStyleSheet( // <<< 新增: 按钮样式
        "QPushButton {"
        "  font-size: 18px;"
        "  padding: 5px 15px;"
        "  color: #333333;" // 深灰色文字
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #BDBDBD);" // 灰色渐变
        "  border: 1px solid #A0A0A0;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #D0D0D0, stop:1 #ADADAD);"
        "}"
        "QPushButton:pressed {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #BDBDBD, stop:1 #E0E0E0);"
        "}"
        );
    connect(skipButton, &QPushButton::clicked, this, &OverlayWidget::skipRequested); // <<< 新增: 连接按钮信号


    countdownLabel = new QLabel(this);
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->setStyleSheet(
        "QLabel {"
        "  color: white;"
        "  font-size: 48px;" // Increased font size
        "  font-weight: bold;"
        "  background-color: transparent;" // Ensure label background is transparent
        "}"
    );

    QVBoxLayout *centralLayout = new QVBoxLayout();
    centralLayout->addStretch(); // 垂直居中内容
    centralLayout->addWidget(countdownLabel, 0, Qt::AlignCenter);
    centralLayout->addSpacing(30); // 标签和按钮之间的间距
    centralLayout->addWidget(skipButton, 0, Qt::AlignCenter);
    centralLayout->addStretch(); // 垂直居中内容

    setLayout(centralLayout);
}

OverlayWidget::~OverlayWidget()
{
    qDebug() << "OverlayWidget for screen" << (targetScreen ? targetScreen->name() : "Unknown") << "destroyed.";
}

void OverlayWidget::setMessage(const QString &message)
{
    countdownLabel->setText(message);
}

void OverlayWidget::updateCountdown(const QString &countdownText)
{
    countdownLabel->setText(countdownText);
}

void OverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // Semi-transparent black background
    painter.fillRect(this->rect(), QColor(34, 139, 34, 190)); // R,G,B,Alpha (190 约 75% 不透明度)
}

void OverlayWidget::keyPressEvent(QKeyEvent *event)
{
    // Prevent closing with Escape key easily.
    // Could add a specific key combo to force close if needed.
    if (event->key() == Qt::Key_Escape) {
        qDebug() << "Escape pressed on overlay, ignoring.";
        event->ignore(); // Or handle it specifically if you want a way to bypass
    } else {
        QWidget::keyPressEvent(event);
    }
}
