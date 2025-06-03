#include "launcher.h"
#include <QApplication>
#include <QDateTime>
#include <QRandomGenerator> // Qt 5.10+


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Launcher w;
    w.show();
    return a.exec();
}
