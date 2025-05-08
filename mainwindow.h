#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ImageViewWidget.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog> // 用于打开文件对话框
#include <QScrollArea> // 可选，如果图片非常大，可以放在滚动区域

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QWidget *centralWidget = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    ImageViewWidget *imageViewer = nullptr;

    QHBoxLayout *buttonLayout = nullptr;
    QPushButton *btnLoad = nullptr;
    QPushButton *btnZoomIn = nullptr;
    QPushButton *btnZoomOut = nullptr;
    QPushButton *btnFit = nullptr;
    QPushButton *btnReset = nullptr;
    QPushButton *btnClear = nullptr;

    QLineEdit   *lineEdit = nullptr;

private slots: // 声明槽函数
    void handlePointsSelected(const QVector<QPointF>& points);

};

#endif // MAINWINDOW_H
