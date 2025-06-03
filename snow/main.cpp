#include "snowwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 对于某些桌面环境（如 Wayland），可能需要显式设置高DPI缩放
    // QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);


    SnowWidget snowEffect;
    snowEffect.show(); // 显示雪花窗口

    return a.exec();
}
