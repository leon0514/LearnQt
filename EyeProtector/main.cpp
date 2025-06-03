#include "settingsdialog.h"
#include <QApplication>
#include <QScreen> // For screen information

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false); // Important for tray icon functionality

    SettingsDialog w;
    // w.show(); // Optionally show settings dialog on start, or let tray icon handle it

    return a.exec();
}