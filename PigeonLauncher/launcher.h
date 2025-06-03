#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QWidget>
#include <QPoint>

class QPushButton;
class QMenu;
class QAction;

class Launcher : public QWidget
{
    Q_OBJECT

public:
    Launcher(QWidget *parent = nullptr);
    ~Launcher();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    // mousePressEvent, mouseMoveEvent, mouseReleaseEvent, contextMenuEvent
    // 可以保留为空或调用基类，因为主要逻辑在 eventFilter 中

private slots:
    void onLaunchPigeonClicked();
    void closeApplication();

private:
    QPushButton *m_launchButton;
    QPoint m_dragPosition;
    bool m_dragging;
    bool m_leftMouseButtonPressedOnButton;

    QMenu *m_contextMenu;
    QAction *m_closeAction;

    void setupUI();
    void createActions();
    void createContextMenu();
};

#endif // LAUNCHER_H
