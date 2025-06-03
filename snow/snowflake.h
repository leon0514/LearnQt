#ifndef SNOWFLAKE_H
#define SNOWFLAKE_H
#include <QRectF>
#include <QString>
#include <QPointF>
#include <QColor>

struct Snowflake {
    QPointF position;    // 雪花当前位置 (x, y)
    double speedY;       // 垂直速度
    double speedX;       // 水平飘移速度 (可以为负)
    int size;            // 雪花字符的像素大小
    QString character;   // 雪花的Unicode字符
    double opacity;      // 透明度 (0.0 - 1.0)
    double initialXOffset; // 用于计算水平飘移的正弦/余弦偏移
    double swayFactor;     // 飘移幅度因子
    double swaySpeed;      // 飘移速度因子

    QColor color;

    Snowflake() : speedY(0), speedX(0), size(0), opacity(1.0), initialXOffset(0), swayFactor(0), swaySpeed(0) {}
};

#endif // SNOWFLAKE_H
