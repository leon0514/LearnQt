QT       += core gui widgets

TARGET = EyeProtector
TEMPLATE = app

SOURCES += main.cpp \
           settingsdialog.cpp \
           overlaywidget.cpp

HEADERS  += settingsdialog.h \
            overlaywidget.h

# If you use a .ui file for SettingsDialog:
# FORMS    += settingsdialog.ui

# If you add icons to a resource file (e.g., for tray icon):
# RESOURCES += resources.qrc

RESOURCES += \
    res.qrc

RC_ICONS=icon/eye.ico
