#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QAbstractButton>
#include <QPropertyAnimation>
#include <QPainter>
#include <QEasingCurve>
#include <QMouseEvent> // For mouseReleaseEvent
#include <QDebug>      // For potential debugging

class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT
    // 声明一个可动画的属性，用于滑块的X位置 (中心点X坐标)
    Q_PROPERTY(int sliderPosition READ sliderPosition WRITE setSliderPosition)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    int sliderPosition() const;
    void setSliderPosition(int position);

    // Customization methods
    void setOnColor(const QColor &color);
    void setOffColor(const QColor &color);
    void setSliderColor(const QColor &color);
    void setHoverColorFactor(const QColor &color); // How much to adjust color on hover
    void setAnimationDuration(int ms);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // Important for recalculating metrics
    // 当控件的选中状态通过 setChecked() 或 toggle() 改变时被调用
    // 我们可以在这里启动动画
    void checkStateSet() override;


private:
    bool m_hovered = false;
    int m_currentSliderPositionX = 0; // 当前滑块中心点的X坐标 (用于动画)
    int m_padding = 3;      // 内边距
    qreal m_sliderRadius;   // 滑块半径 (qreal for precision)
    qreal m_trackHeight;    // 轨道高度
    // m_trackWidth is effectively (width() - 2 * m_padding)

    QColor m_onColor = QColor(0, 180, 0);   // 选中时的背景颜色 (绿色)
    QColor m_offColor = QColor(180, 180, 180); // 未选中时的背景颜色 (灰色)
    QColor m_sliderColor = Qt::white;     // 滑块颜色
    QColor m_hoverColorAdjustment = QColor(20, 20, 20); // 悬停时颜色调整因子 (additive)

    QPropertyAnimation *m_animation;

    void calculateMetrics(); // 根据当前大小计算绘图参数
    int getTargetSliderPosition() const; // Helper to get target X based on state
};

#endif // TOGGLESWITCH_H
