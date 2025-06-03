#ifndef SNOWWIDGET_H
#define SNOWWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <random> // C++11 随机数
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "snowflake.h"

class SnowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SnowWidget(QWidget *parent = nullptr);
    ~SnowWidget();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // 当窗口大小改变时（虽然我们是全屏，但初始化时有用）

private slots:
    void updateSnowflakes();

private:
    void initializeSnowflakes();
    QRect getCombinedScreenGeometry(); // 获取所有屏幕的联合区域

    QVector<Snowflake> m_snowflakes;
    QTimer *m_animationTimer;

    QList<QString> m_unicodeSnowChars;
    QList<int> m_snowSizes;

    // C++11 随机数生成器
    std::mt19937 m_rng; // Mersenne Twister 随机数引擎

    QRect m_screenBoundingRect; // 屏幕的总边界
    int m_numSnowflakes;

    void setupTrayIcon();
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
};

#endif // SNOWWIDGET_H
