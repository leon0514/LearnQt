#include "launcher.h"
#include "pigeonwidget.h" // 重要: 包含PigeonWidget头文件
#include <QPushButton>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QIcon>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QApplication> // For qApp
#include <QDebug>
#include <QScreen>

const int BUTTON_SIZE = 80; // Launcher按钮的大小

Launcher::Launcher(QWidget *parent)
    : QWidget(parent),
    m_dragging(false),
    m_leftMouseButtonPressedOnButton(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(BUTTON_SIZE, BUTTON_SIZE);

    setupUI();
    createActions();
    createContextMenu();

    setWindowTitle("Pigeon Launcher"); // 用于PigeonWidget中可能的查找（虽然现在不这么做了）
}

Launcher::~Launcher()
{
}

void Launcher::setupUI()
{
    m_launchButton = new QPushButton(this);
    m_launchButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    m_launchButton->setText("");
    m_launchButton->setFocusPolicy(Qt::NoFocus);

    m_launchButton->installEventFilter(this); // 安装事件过滤器

    QIcon launchIcon(":/images/fly.png");
    if (launchIcon.isNull()) {
        qWarning("Failed to load launch icon. Using fallback text 'L'.");
        m_launchButton->setText("L");
        m_launchButton->setFont(QFont("Arial", BUTTON_SIZE / 2, QFont::Bold));
    } else {
        m_launchButton->setIcon(launchIcon);
        m_launchButton->setIconSize(QSize(BUTTON_SIZE * 0.7, BUTTON_SIZE * 0.7));
    }

    QString qss = QString(
                      "QPushButton {"
                      "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #88aaff, stop:1 #5588dd);"
                      "   border: none;" // 通常圆形图标按钮不需要边框
                      "   border-radius: %1px;"
                      "}"
                      "QPushButton:hover {"
                      "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #99bbff, stop:1 #6699ee);"
                      "}"
                      "QPushButton:pressed {"
                      "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #6699dd, stop:1 #3366bb);"
                      "}"
                      ).arg(BUTTON_SIZE / 2);

    m_launchButton->setStyleSheet(qss);
    m_launchButton->move(0,0); // 因为Launcher窗口大小和按钮一样

    connect(m_launchButton, &QPushButton::clicked, this, &Launcher::onLaunchPigeonClicked);
}

void Launcher::createActions()
{
    m_closeAction = new QAction(tr("&Close Launcher"), this);
    m_closeAction->setShortcut(tr("Ctrl+Q"));
    connect(m_closeAction, &QAction::triggered, this, &Launcher::closeApplication);
}

void Launcher::createContextMenu()
{
    m_contextMenu = new QMenu(this);
    m_contextMenu->addAction(m_closeAction);
}

void Launcher::closeApplication()
{
    qApp->quit();
}

void Launcher::onLaunchPigeonClicked()
{
    qDebug() << "Launch button clicked!";

    // 获取 Launcher 按钮所在的屏幕
    QScreen *targetScreen = this->screen(); // 获取 Launcher 当前所在的屏幕
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen(); // 后备：如果获取失败，使用主屏幕
        qWarning("Launcher::onLaunchPigeonClicked - Could not get current screen, using primary screen.");
    }
    QRect screenGeometry = targetScreen->availableGeometry(); // 使用目标屏幕的可用几何

    // 获取 Launcher 窗口 (即按钮) 的中心点，并转换为全局坐标
    QPoint launcherCenterGlobal = this->mapToGlobal(this->rect().center());

    // 计算鸽子窗口的左上角位置，使其中心与按钮中心对齐
    QPoint pigeonTopLeft(launcherCenterGlobal.x() - PigeonWidget::DEFAULT_PIGEON_SIZE / 2,
                         launcherCenterGlobal.y() - PigeonWidget::DEFAULT_PIGEON_SIZE / 2);

    // 创建 PigeonWidget 并传递起始位置 和 目标屏幕指针
    for (int i = 0; i < 10; i++)
    {
        PigeonWidget *pigeon = new PigeonWidget(pigeonTopLeft, targetScreen, nullptr); // 传递屏幕指针
        pigeon->show();
    }

}

bool Launcher::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_launchButton) {
        QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);
        QContextMenuEvent *contextEvent = dynamic_cast<QContextMenuEvent*>(event);

        if (event->type() == QEvent::MouseButtonPress) {
            if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
                m_dragPosition = mouseEvent->globalPosition().toPoint() - this->frameGeometry().topLeft();
                m_leftMouseButtonPressedOnButton = true;
                return false; // 让按钮能处理点击
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (mouseEvent && (mouseEvent->buttons() & Qt::LeftButton) && m_leftMouseButtonPressedOnButton) {
                if (!m_dragging && (mouseEvent->globalPosition().toPoint() - this->frameGeometry().topLeft() - m_dragPosition).manhattanLength() > QApplication::startDragDistance()) {
                    m_dragging = true;
                }
                if (m_dragging) {
                    move(mouseEvent->globalPosition().toPoint() - m_dragPosition);
                    return true; // 拖动时消耗事件
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (mouseEvent && mouseEvent->button() == Qt::LeftButton && m_leftMouseButtonPressedOnButton) {
                m_leftMouseButtonPressedOnButton = false;
                if (m_dragging) {
                    m_dragging = false;
                    return true; // 拖动结束，消耗事件
                }
                return false; // 非拖动，让按钮处理点击
            }
        } else if (event->type() == QEvent::ContextMenu && contextEvent) {
            m_contextMenu->exec(contextEvent->globalPos());
            return true; // 已处理上下文菜单
        }
    }
    return QWidget::eventFilter(watched, event);
}
