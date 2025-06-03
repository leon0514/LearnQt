#include "pigeonwidget.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QScreen>
#include <QGuiApplication>
#include <QVBoxLayout>
#include <cmath> // For M_PI, cos, sin, std::hypot, std::max, std::min
#include <QDebug>

// --- 常量定义 ---
// 使用 PigeonWidget::DEFAULT_PIGEON_SIZE 代替独立的 PIGEON_SIZE

const int MOVE_INTERVAL_MS = 30;
const int ANIMATION_INTERVAL_MS = 150;
const int MAX_LIFESPAN_FRAMES_CONST = 700;
const double MIN_SPEED_CONST = 4.0;
const double MAX_SPEED_CONST = 8.0;

// 角度定义 (关键)
const double INITIAL_LEFT_UP_MIN_ANGLE_RAD = M_PI + M_PI / 12.0;           // 195 度
const double INITIAL_LEFT_UP_MAX_ANGLE_RAD = (3.0 * M_PI / 2.0) - M_PI / 12.0; // 255 度
const double FLIGHT_CLAMP_MIN_ANGLE_RAD = M_PI + M_PI / 18.0;              // 190 度
const double FLIGHT_CLAMP_MAX_ANGLE_RAD = (3.0 * M_PI / 2.0) - M_PI / 18.0;    // 260 度

// 飞行过程中的扰动参数
const double MAX_ANGLE_CHANGE_PER_STEP = M_PI / 72.0; // +/- 2.5度
const double MAX_SPEED_CHANGE_PER_STEP = 0.08;


PigeonWidget::PigeonWidget(const QPoint &startPos, QScreen *targetScreen, QWidget *parent)
    : QWidget(parent),
    m_currentFrameIndex(0),
    m_lifeSpanFrames(MAX_LIFESPAN_FRAMES_CONST),
    m_initialStartPosition(startPos),
    m_targetScreen(targetScreen) // 保存目标屏幕
{
    // 如果没有传递有效的屏幕，则使用鸽子当前（或将要显示）的屏幕，或者主屏幕作为后备
    if (!m_targetScreen) {
        // 当窗口还未显示时，this->screen() 可能返回 nullptr
        // 通常在show()之后，this->screen()才会可靠
        // 所以，如果构造时 targetScreen 是 nullptr，我们最好在 movePigeon 中动态获取
        // 或者依赖主屏幕
        qWarning("PigeonWidget constructor: targetScreen is null, will attempt to use primary or current screen later.");
        // m_targetScreen = QGuiApplication::primaryScreen(); // 可以在这里设置一个后备
    }


    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    loadAnimationFrames();

    if (m_wingAnimationFrames.isEmpty()) {
        qWarning("No animation frames loaded! Using fallback red square.");
        QPixmap fallback(DEFAULT_PIGEON_SIZE, DEFAULT_PIGEON_SIZE);
        fallback.fill(Qt::red);
        m_wingAnimationFrames.append(fallback);
    }

    m_pigeonLabel = new QLabel(this);
    if (!m_wingAnimationFrames.isEmpty()) {
        m_pigeonLabel->setPixmap(m_wingAnimationFrames.first());
    } else {
        QPixmap emptyPixmap(DEFAULT_PIGEON_SIZE, DEFAULT_PIGEON_SIZE);
        emptyPixmap.fill(Qt::transparent);
        m_pigeonLabel->setPixmap(emptyPixmap);
    }
    m_pigeonLabel->setFixedSize(DEFAULT_PIGEON_SIZE, DEFAULT_PIGEON_SIZE);
    setFixedSize(DEFAULT_PIGEON_SIZE, DEFAULT_PIGEON_SIZE);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_pigeonLabel);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    initPigeon();

    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, &PigeonWidget::movePigeon);
    m_moveTimer->start(MOVE_INTERVAL_MS);

    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &PigeonWidget::animateWings);
    m_animationTimer->start(ANIMATION_INTERVAL_MS);
}

PigeonWidget::~PigeonWidget()
{
    qDebug("Pigeon destroyed");
}

void PigeonWidget::loadAnimationFrames()
{
    QStringList framePaths;
    // 确保你有这两张不同的图片用于动画
    framePaths << ":/images/bird.png"
               << ":/images/bird.png";

    for (const QString &path : framePaths) {
        QPixmap frame;
        if (frame.load(path)) {
            m_wingAnimationFrames.append(frame.scaled(DEFAULT_PIGEON_SIZE, DEFAULT_PIGEON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            qWarning() << "Failed to load animation frame:" << path;
        }
    }
    if (m_wingAnimationFrames.size() < 2 && !m_wingAnimationFrames.isEmpty()) {
        qWarning("Animation requires at least two different frames for visual effect. Duplicating first frame.");
        if(!m_wingAnimationFrames.isEmpty()) m_wingAnimationFrames.append(m_wingAnimationFrames.first());
    }
}

void PigeonWidget::animateWings()
{
    if (m_wingAnimationFrames.size() < 2) return; // 少于2帧没有动画意义
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_wingAnimationFrames.size();
    m_pigeonLabel->setPixmap(m_wingAnimationFrames.at(m_currentFrameIndex));
}

void PigeonWidget::initPigeon() // 这个函数现在主要设置初始位置和方向
{
    move(m_initialStartPosition);
    setInitialDirectionAndSpeed(); // 方向和速度不直接依赖屏幕几何
}

void PigeonWidget::setInitialDirectionAndSpeed()
{
    double speed = QRandomGenerator::global()->generateDouble() * (MAX_SPEED_CONST - MIN_SPEED_CONST) + MIN_SPEED_CONST;
    double angle = QRandomGenerator::global()->generateDouble() *
                       (INITIAL_LEFT_UP_MAX_ANGLE_RAD - INITIAL_LEFT_UP_MIN_ANGLE_RAD) +
                   INITIAL_LEFT_UP_MIN_ANGLE_RAD;

    m_velocity.setX(speed * std::cos(angle));
    m_velocity.setY(speed * std::sin(angle));

    // 最终保证速度分量符合要求
    if (m_velocity.x() > -0.01 * speed) m_velocity.setX(std::min(-0.01 * speed, m_velocity.x() - 0.05 * speed));
    if (m_velocity.y() > -0.01 * speed) m_velocity.setY(std::min(-0.01 * speed, m_velocity.y() - 0.05 * speed));

    qDebug() << "Initial STRICT Left-Up Pigeon - Angle (deg):" << (angle * 180.0 / M_PI)
             << "Speed:" << speed
             << "Velocity:" << m_velocity;
}

void PigeonWidget::movePigeon()
{
    // 1. 速度和角度的扰动逻辑 (保持不变)
    // ... (之前的代码) ...
    double currentSpeed = std::hypot(m_velocity.x(), m_velocity.y());
    double currentAngle = std::atan2(m_velocity.y(), m_velocity.x());
    double angleChange = (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * MAX_ANGLE_CHANGE_PER_STEP;
    double speedChange = (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * MAX_SPEED_CHANGE_PER_STEP;
    double newAngle = currentAngle + angleChange;
    double newSpeed = currentSpeed + speedChange;
    newSpeed = std::max(MIN_SPEED_CONST * 0.8, std::min(newSpeed, MAX_SPEED_CONST * 1.2));

    double targetVx = newSpeed * std::cos(newAngle);
    double targetVy = newSpeed * std::sin(newAngle);
    bool vxOk = (targetVx < -1e-3);
    bool vyOk = (targetVy < -1e-3);

    if (!vxOk || !vyOk) {
        newAngle = QRandomGenerator::global()->generateDouble() *
                       (FLIGHT_CLAMP_MAX_ANGLE_RAD - FLIGHT_CLAMP_MIN_ANGLE_RAD) +
                   FLIGHT_CLAMP_MIN_ANGLE_RAD;
    }
    m_velocity.setX(newSpeed * std::cos(newAngle));
    m_velocity.setY(newSpeed * std::sin(newAngle));

    if (m_velocity.x() > -0.01 * newSpeed) m_velocity.setX(std::min(-0.01 * newSpeed, m_velocity.x() - 0.05 * newSpeed));
    if (m_velocity.y() > -0.01 * newSpeed) m_velocity.setY(std::min(-0.01 * newSpeed, m_velocity.y() - 0.05 * newSpeed));


    // 2. 更新位置 (保持不变)
    QPointF currentPos = this->pos();
    currentPos += m_velocity;
    move(currentPos.toPoint());

    // 3. 减少生命周期 (保持不变)
    m_lifeSpanFrames--;

    // 4. 边界检测 (核心修改区域)
    bool outOfBounds = true; // 先假设出界

    // 获取鸽子当前的几何（全局坐标）
    QRect pigeonGlobalRect = this->geometry();

    // 遍历所有连接的屏幕
    QList<QScreen*> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (!screen) continue;

        QRect screenAvailableRect = screen->availableGeometry(); // 这是全局坐标
        // 创建一个扩展的屏幕区域用于判断，确保鸽子完全飞出一段距离才消失
        QRect expandedScreenRect = screenAvailableRect.adjusted(
            -this->width() * 1,  // 向外扩展鸽子宽度的1倍 (可以调整倍数)
            -this->height() * 1, // 向外扩展鸽子高度的1倍
            this->width() * 1,
            this->height() * 1
            );

        if (expandedScreenRect.intersects(pigeonGlobalRect)) {
            outOfBounds = false; // 鸽子与至少一个扩展屏幕区域相交，认为它在界内
            // qDebug() << "Pigeon intersects with screen:" << screen->name() << "Expanded:" << expandedScreenRect << "Pigeon:" << pigeonGlobalRect;
            break; // 只要与一个屏幕相交即可，不需要继续检查其他屏幕
        }
    }

    if (outOfBounds || m_lifeSpanFrames <= 0) {
        qDebug() << "Pigeon closing. Out of bounds:" << outOfBounds << "Lifespan:" << m_lifeSpanFrames << "Pos:" << this->pos();
        m_moveTimer->stop();
        m_animationTimer->stop();
        close();
    }
}
