#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
// #include <QLineEdit> // 如果不再需要，可以移除
#include <QFileDialog>
#include <QTableWidget>
#include <QLabel>
#include <QThread>        // 添加 QThread 头文件
#include "xmlprocessor.h" // 添加 XmlProcessor 头文件

// 如果 vocParser.h 的完整定义在这里不是必需的，可以前向声明
// class VocParser; // 如果 XmlProcessor 完全处理它，则不需要

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QWidget *centralWidget = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    QHBoxLayout *buttonLayout = nullptr;
    QSpacerItem *space = nullptr;

    QPushButton *btnLoad    = nullptr;
    QPushButton *btnAnalyze = nullptr;
    QPushButton *btnAnalyzeBox = nullptr;

    QLabel *labelDir = nullptr;

    QTableWidget *tabelWidget = nullptr;
    QTableWidgetItem *totalItem = nullptr;

    QTableWidget *labelTabelWidget = nullptr;

private:
    void setupUi();
    void connect_all();


private slots:
    // 处理按钮点击的槽函数
    void handleLoadXml();
    void handleAnalyzeDistribution();
    void handleAnalyzeBoxCounts();

    // 更新UI的槽函数，由工作线程的信号触发
    void updateDistributionTable(const QVector<int>& counts, int totalFilesProcessed);
    void updateBoxCountTable(const QMap<QString, int>& boxMap);
    void onProcessingStarted();  // 用于禁用按钮
    void onProcessingFinished(); // 用于重新启用按钮

private:
    QString xml_dir_;
    QVector<QString> xml_list_;

    // VocParser parser_; // VocParser 实例将移至工作者线程

    QThread *workerThread_ = nullptr;         // 添加工作线程指针
    XmlProcessor *xmlProcessor_ = nullptr; // 添加工作者对象指针

signals: // 用于触发工作者槽函数的信号
    void requestXmlDistributionProcessing(const QVector<QString>& xmlFiles);
    void requestXmlBoxCountProcessing(const QVector<QString>& xmlFiles);

};


#endif // MAINWINDOW_H
