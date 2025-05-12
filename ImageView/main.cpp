#include "mainwindow.h"

#include <QApplication>
#include <QMainWindow>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(":/logo.ico"));

    // 加载 QSS 文件
    QFile styleFile(":/style.qss");

    if (!styleFile.open(QFile::ReadOnly | QFile::Text))
    {
        qWarning() << "Cannot open stylesheet file:" << styleFile.errorString();
    }
    else
    {
        QString styleSheetContent = QTextStream(&styleFile).readAll();
        a.setStyleSheet(styleSheetContent);
        styleFile.close();
    }

    w.show();
    return a.exec();
}
