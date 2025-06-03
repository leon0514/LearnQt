#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QList>
#include <QSpinBox>
#include <QPushButton>
#include <QSystemTrayIcon> // Optional
#include <QSettings>       // Optional
#include <QCheckBox>

class OverlayWidget; // Forward declaration

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void applySettings();
    void startReminderTimer();
    void showOverlay();
    void updateCountdown();
    void hideOverlay();

    // Optional: Tray icon slots
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showSettings();
    void quitApplication();


protected:
    void closeEvent(QCloseEvent *event) override; // To hide instead of close if tray icon exists

private:
    Ui::SettingsDialog *ui; // If using .ui file
    // Or define UI elements manually if not using .ui
    QSpinBox *reminderIntervalSpinBox;
    QSpinBox *countdownDurationSpinBox;
    QPushButton *applyButton;
    QPushButton *startButton; // To manually start/restart the timer

    QTimer *reminderTimer;
    QTimer *countdownTimer;

    int reminderIntervalMinutes;
    int breakDurationSeconds;
    int currentCountdownSeconds;

    QList<OverlayWidget*> overlayWidgets;

    // Optional: Tray Icon
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;

    // Optional: Settings persistence
    QSettings *appSettings;
    void loadSettings();
    void saveSettings();

    void createUiManually(); // Call this if not using .ui file
    void createTrayIcon();   // Optional

    QCheckBox *autoStartCheckBox;
    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;
};

#endif // SETTINGSDIALOG_H
