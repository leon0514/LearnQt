#ifndef PIGEONWIDGET_H
#define PIGEONWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QPointF>
#include <QPoint>
#include <QLabel>
#include <QList>

class QScreen; // 前向声明 QScreen

class PigeonWidget : public QWidget
{
    Q_OBJECT

public:
    static const int DEFAULT_PIGEON_SIZE = 64;
    // 修改构造函数以接受初始位置和目标屏幕
    explicit PigeonWidget(const QPoint &startPos, QScreen *targetScreen, QWidget *parent = nullptr);
    ~PigeonWidget();

private slots:
    void movePigeon();
    void animateWings();

private:
    QLabel *m_pigeonLabel;
    QList<QPixmap> m_wingAnimationFrames;
    int m_currentFrameIndex;
    QTimer *m_moveTimer;
    QTimer *m_animationTimer;
    QPointF m_velocity;
    int m_lifeSpanFrames;
    QPoint m_initialStartPosition;
    QScreen *m_targetScreen; // 存储目标屏幕

    void initPigeon();
    void setInitialDirectionAndSpeed(); // 这个函数不需要屏幕信息，因为它只设置速度向量
    void loadAnimationFrames();
};

#endif // PIGEONWIDGET_H
