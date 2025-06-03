#include "settingsdialog.h"
#include "overlaywidget.h" // Include after OverlayWidget is fully defined
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGuiApplication>
#include <QScreen>
#include <QMessageBox>
#include <QCloseEvent>
#include <QMenu> // For tray menu
#include <QStyle>
#include <QApplication>

#include <QFormLayout>
#include <QGroupBox>
#include <QSpacerItem>
#include <QDir>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    createUiManually();

    setWindowIcon(QIcon(":/icon/eye.ico"));
    reminderTimer = new QTimer(this);
    countdownTimer = new QTimer(this);

    connect(reminderTimer, &QTimer::timeout, this, &SettingsDialog::showOverlay);
    connect(countdownTimer, &QTimer::timeout, this, &SettingsDialog::updateCountdown);

    // Optional: Settings persistence
    appSettings = new QSettings("Leon", "EyeProtector", this);
    loadSettings();
    createTrayIcon();

    applySettings();
}



SettingsDialog::~SettingsDialog()
{
    // if (ui) delete ui; // If using .ui file
    // Timers are children, will be deleted automatically
    // OverlayWidgets should be deleted in hideOverlay or destructor
    qDeleteAll(overlayWidgets);
    overlayWidgets.clear();
}

void SettingsDialog::createUiManually() {
    // 主垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15); // 设置窗口的边距
    mainLayout->setSpacing(10);                     // 设置主布局中控件之间的间距

    // 1. 设置分组框
    QGroupBox *settingsGroup = new QGroupBox("自定义设置", this);
    settingsGroup->setStyleSheet("QGroupBox { font-weight: bold; }"); // 可选：加粗组标题

    // 2. 使用 QFormLayout 来排列标签和输入框
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight); // 标签右对齐
    formLayout->setHorizontalSpacing(10);          // 标签和输入框之间的水平间距
    formLayout->setVerticalSpacing(8);             // 行之间的垂直间距
    formLayout->setContentsMargins(10, 15, 10, 10); // 组框内部的边距

    // 提醒间隔
    QLabel *reminderLabel = new QLabel("提醒间隔 (分钟):", settingsGroup);
    reminderIntervalSpinBox = new QSpinBox(settingsGroup);
    reminderIntervalSpinBox->setRange(1, 120);
    reminderIntervalSpinBox->setValue(15);
    reminderIntervalSpinBox->setSuffix(" 分钟"); // 添加单位后缀
    reminderIntervalSpinBox->setToolTip("设置多久提醒一次（1-120分钟）");
    // reminderIntervalSpinBox->setFixedWidth(100); // 如果需要可以固定宽度

    formLayout->addRow(reminderLabel, reminderIntervalSpinBox);

    // 倒计时时长
    QLabel *countdownLabel = new QLabel("休息倒计时 (秒):", settingsGroup);
    countdownDurationSpinBox = new QSpinBox(settingsGroup);
    countdownDurationSpinBox->setRange(5, 300);
    countdownDurationSpinBox->setValue(20);
    countdownDurationSpinBox->setSuffix(" 秒"); // 添加单位后缀
    countdownDurationSpinBox->setToolTip("设置提醒时屏幕锁定的时间（5-300秒）");
    // countdownDurationSpinBox->setFixedWidth(100); // 如果需要可以固定宽度

    autoStartCheckBox = new QCheckBox("开机自动启动", settingsGroup);
    autoStartCheckBox->setToolTip("启用后程序将在系统启动时自动运行");
    // 初始化复选框状态



    formLayout->addRow(countdownLabel, countdownDurationSpinBox);

    settingsGroup->setLayout(formLayout); // 将表单布局设置到组框中

    // 3. 应用按钮
    applyButton = new QPushButton("应用设置并启动", this);
    applyButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton)); // 添加一个标准图标
    applyButton->setMinimumHeight(30); // 设置按钮最小高度
    applyButton->setToolTip("保存当前设置并启动/重启提醒计时器");
    connect(applyButton, &QPushButton::clicked, this, &SettingsDialog::applySettings);

    autoStartCheckBox->setChecked(isAutoStartEnabled());
    formLayout->addRow(autoStartCheckBox);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1); // 添加弹性空间，将按钮推向右侧
    buttonLayout->addWidget(applyButton);
    buttonLayout->addStretch(1); // 如果想居中，两边都加 addStretch

    // 将各部分添加到主布局
    mainLayout->addWidget(settingsGroup);
    mainLayout->addSpacerItem(new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding)); // 在设置和按钮间添加一点垂直弹性空间
    mainLayout->addLayout(buttonLayout); // 添加按钮布局，而不是直接添加按钮

    setLayout(mainLayout);

    setMinimumWidth(350);
}

// 实现开机启动设置函数
void SettingsDialog::setAutoStart(bool enable) {
    QString appName = QCoreApplication::applicationName();
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

#if defined(Q_OS_WIN)
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enable) {
        // 路径包含空格时需要添加引号
        settings.setValue(appName, "\"" + appPath + "\"");
    } else {
        settings.remove(appName);
    }

#elif defined(Q_OS_LINUX)
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/";
    QString desktopPath = autostartDir + appName + ".desktop";

    if (enable) {
        QDir().mkpath(autostartDir);
        QFile file(desktopPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Type=Application\n"
                << "Exec=" << appPath << "\n"
                << "Name=" << appName << "\n"
                << "Comment=Start " << appName << " at login\n"
                << "X-GNOME-Autostart-enabled=true\n";
            file.close();
        }
    } else {
        QFile::remove(desktopPath);
    }

#elif defined(Q_OS_MACOS)
    QString plistDir = QDir::homePath() + "/Library/LaunchAgents/";
    QString plistPath = plistDir + "com." + appName + ".plist";

    if (enable) {
        QDir().mkpath(plistDir);
        QFile file(plistPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                << "<plist version=\"1.0\">\n"
                << "<dict>\n"
                << "    <key>Label</key>\n"
                << "    <string>com." << appName << "</string>\n"
                << "    <key>ProgramArguments</key>\n"
                << "    <array>\n"
                << "        <string>" << appPath << "</string>\n"
                << "    </array>\n"
                << "    <key>RunAtLoad</key>\n"
                << "    <true/>\n"
                << "</dict>\n"
                << "</plist>";
            file.close();
        }
    } else {
        QFile::remove(plistPath);
    }
#endif
}

// 实现检查开机启动状态的函数
bool SettingsDialog::isAutoStartEnabled() const {
    QString appName = QCoreApplication::applicationName();

#if defined(Q_OS_WIN)
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return settings.contains(appName);

#elif defined(Q_OS_LINUX)
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/";
    QString desktopPath = autostartDir + appName + ".desktop";
    return QFile::exists(desktopPath);

#elif defined(Q_OS_MACOS)
    QString plistDir = QDir::homePath() + "/Library/LaunchAgents/";
    QString plistPath = plistDir + "com." + appName + ".plist";
    return QFile::exists(plistPath);

#else
    return false;
#endif
}


void SettingsDialog::applySettings()
{
    setAutoStart(autoStartCheckBox->isChecked());

    reminderIntervalMinutes = reminderIntervalSpinBox->value();
    breakDurationSeconds = countdownDurationSpinBox->value();

    if (reminderIntervalMinutes <= 0 || breakDurationSeconds <= 0) {
        QMessageBox::warning(this, "无效设置", "时间必须大于0。");
        return;
    }

    saveSettings(); // Optional: Save current settings
    startReminderTimer();
    if (trayIcon) { // Hide settings window if tray icon exists, timer will run in background
        hide();
    }

    QMessageBox msgBox(this); // 'this' 是父窗口指针
    msgBox.setWindowIcon(QIcon(":/icon/eye.ico"));
    msgBox.setWindowTitle("");
    msgBox.setText(QString("提醒已启动。\n每隔 %1 分钟提醒一次，休息 %2 秒。").arg(reminderIntervalMinutes).arg(breakDurationSeconds));
    msgBox.setIconPixmap(QPixmap(":/icon/eye.ico")); // 或者自定义图标

    // msgBox.setStandardButtons(QMessageBox::Ok); // 设置按钮
    // msgBox.setDefaultButton(QMessageBox::Ok);   // 设置默认按钮
    msgBox.exec();

}

void SettingsDialog::startReminderTimer()
{
    reminderTimer->stop();
    reminderTimer->start(reminderIntervalMinutes * 60 * 1000); // Convert minutes to milliseconds
    qDebug() << "Reminder timer started for" << reminderIntervalMinutes << "minutes.";
}

void SettingsDialog::showOverlay()
{
    qDebug() << "Reminder triggered! Showing overlay.";
    reminderTimer->stop(); // 停止主提醒计时器

    if (!overlayWidgets.isEmpty()) {
        qDebug() << "Warning: overlayWidgets list was not empty when showOverlay was called. Cleaning up...";
        hideOverlay();      // 这会调用 delete overlayWidgets 然后清空列表
        reminderTimer->stop(); // 再次停止，因为 hideOverlay 会重启它
    }

    QList<QScreen*> screens = QGuiApplication::screens();
    for (QScreen *screen : std::as_const(screens)) {
        OverlayWidget *overlay = new OverlayWidget(screen);
        overlay->setMessage(QString("保护视力，休息一下！\n%1 秒后自动消失").arg(breakDurationSeconds));

        // <<< 新增: 连接 OverlayWidget 的 skipRequested 信号到 SettingsDialog 的 hideOverlay 槽 >>>
        connect(overlay, &OverlayWidget::skipRequested, this, &SettingsDialog::hideOverlay);

        overlay->showFullScreen();
        overlayWidgets.append(overlay);
    }

    currentCountdownSeconds = breakDurationSeconds;
    updateCountdown(); // 立即显示初始倒计时
    countdownTimer->start(1000); // 每秒更新一次
}

void SettingsDialog::hideOverlay()
{
    qDebug() << "Hiding overlay.";
    countdownTimer->stop();
    for (OverlayWidget *overlay : std::as_const(overlayWidgets)) {
        // 断开连接，以防在析构过程中意外触发信号
        disconnect(overlay, &OverlayWidget::skipRequested, this, &SettingsDialog::hideOverlay);
        overlay->close();
        delete overlay;
    }
    overlayWidgets.clear();

    startReminderTimer(); // 重新启动主提醒计时器
}


void SettingsDialog::updateCountdown()
{
    if (currentCountdownSeconds <= 0) {
        hideOverlay();
        return;
    }

    QString message = QString("休息倒计时: %1 秒").arg(currentCountdownSeconds);
    for (OverlayWidget *overlay : std::as_const(overlayWidgets)) {
        overlay->updateCountdown(message);
    }
    qDebug() << "Countdown:" << currentCountdownSeconds;
    currentCountdownSeconds--;
}


void SettingsDialog::loadSettings()
{
    if (!appSettings) return;
    reminderIntervalSpinBox->setValue(appSettings->value("reminderInterval", 15).toInt());
    countdownDurationSpinBox->setValue(appSettings->value("breakDuration", 20).toInt());
    // Apply these loaded settings
    reminderIntervalMinutes = reminderIntervalSpinBox->value();
    breakDurationSeconds = countdownDurationSpinBox->value();
}

void SettingsDialog::saveSettings()
{
    if (!appSettings) return;
    appSettings->setValue("reminderInterval", reminderIntervalMinutes);
    appSettings->setValue("breakDuration", breakDurationSeconds);
}

void SettingsDialog::createTrayIcon() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning("System tray not available on this system.");
        // Fallback: show the dialog normally if tray is not available
        show();
        return;
    }

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icon/eye.ico"));

    trayMenu = new QMenu(this);
    QAction *showAction = trayMenu->addAction("显示设置");
    connect(showAction, &QAction::triggered, this, &SettingsDialog::showSettings);

    QAction *quitAction = trayMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, this, &SettingsDialog::quitApplication);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("护眼提醒");

    connect(trayIcon, &QSystemTrayIcon::activated, this, &SettingsDialog::trayIconActivated);

    trayIcon->show();
}

void SettingsDialog::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) { // Typically left click
        showSettings();
    }
}

void SettingsDialog::showSettings() {
    this->showNormal(); // Show the dialog
    this->raise();      // Bring to front
    this->activateWindow();
}

void SettingsDialog::quitApplication() {
    QApplication::quit();
}

void SettingsDialog::closeEvent(QCloseEvent *event) {
    if (trayIcon && trayIcon->isVisible()) {
        hide(); // Hide to tray instead of closing
        event->ignore();
    } else {
        // If no tray icon, or tray icon somehow not visible, proceed with default close
        // This also handles quitting if the tray icon's "Quit" action is used.
        // Make sure timers are stopped to prevent issues after main event loop ends.
        reminderTimer->stop();
        countdownTimer->stop();
        hideOverlay(); // Clean up any visible overlays
        event->accept();
        QApplication::quit(); // Ensure app quits fully if window is closed and no tray
    }
}
