#include "comparewidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CompareWidget w;

    w.setWindowTitle("Compare Widget Test");
    w.setMinimumSize(600, 400);

    w.show();
    return a.exec();
}
