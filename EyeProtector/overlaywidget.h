#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QScreen>
#include <QPushButton>

class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayWidget(QScreen *screen, QWidget *parent = nullptr);
    ~OverlayWidget();

    void setMessage(const QString &message);
    void updateCountdown(const QString &countdownText);

signals:
    void skipRequested(); // <<< 信号，当用户点击提前结束时发出

protected:
    void paintEvent(QPaintEvent *event) override; // For custom background
    void keyPressEvent(QKeyEvent *event) override; // To prevent closing with Esc easily

private:
    QLabel *countdownLabel;
    QPushButton *skipButton;
    QScreen *targetScreen; // The screen this overlay is for
};

#endif // OVERLAYWIDGET_H
