#include "mainwindow.h"
#include <QHeaderView>
#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>        // 用于 qDebug 输出
#include <QMessageBox>   // 用于用户反馈

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->setWindowTitle("统计标签");
    this->resize(800, 600);

    setupUi(); // 封装UI创建过程

    setWindowIcon(QIcon(":/count.ico"));

    // --- 线程和工作者设置 ---
    workerThread_ = new QThread(this); // 父对象设为 this，以便在 MainWindow 析构时自动清理（如果MainWindow是堆分配的）
    xmlProcessor_ = new XmlProcessor(); // 不设置父对象，因为它将被移动到线程
    xmlProcessor_->moveToThread(workerThread_); // 将工作者对象移动到新线程

    // 连接 MainWindow 的信号到 XmlProcessor 的槽 (用于触发工作)
    // Qt::QueuedConnection 是跨线程连接的默认方式，确保槽函数在接收者所在线程执行
    connect(this, &MainWindow::requestXmlDistributionProcessing, xmlProcessor_, &XmlProcessor::processXmlDistribution);
    connect(this, &MainWindow::requestXmlBoxCountProcessing, xmlProcessor_, &XmlProcessor::processXmlBoxCounts);

    // 连接 XmlProcessor 的信号到 MainWindow 的槽 (用于UI更新)
    connect(xmlProcessor_, &XmlProcessor::distributionProcessingFinished, this, &MainWindow::updateDistributionTable);
    connect(xmlProcessor_, &XmlProcessor::boxCountProcessingFinished, this, &MainWindow::updateBoxCountTable);
    connect(xmlProcessor_, &XmlProcessor::processingStarted, this, &MainWindow::onProcessingStarted);
    connect(xmlProcessor_, &XmlProcessor::processingFinished, this, &MainWindow::onProcessingFinished);

    // 当线程结束时，安排删除工作者对象
    connect(workerThread_, &QThread::finished, xmlProcessor_, &QObject::deleteLater);

    workerThread_->start(); // 启动工作线程的事件循环

    connect_all(); // 连接按钮的点击事件等
}

void MainWindow::setupUi() {
    centralWidget = new QWidget(this);
    mainLayout    = new QVBoxLayout(centralWidget);

    space = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    buttonLayout  = new QHBoxLayout();
    btnLoad       = new QPushButton("xml路径", centralWidget);
    btnAnalyze    = new QPushButton("统计标签分布", centralWidget);
    btnAnalyzeBox = new QPushButton("统计标签个数", centralWidget);

    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnLoad);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnAnalyze);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnAnalyzeBox);
    buttonLayout->addItem(space);
    mainLayout->addLayout(buttonLayout);

    labelDir = new QLabel("当前选择目录 : ", centralWidget);
    mainLayout->addWidget(labelDir);

    tabelWidget = new QTableWidget(6, 3, centralWidget);
    tabelWidget->setHorizontalHeaderLabels(QStringList() << "标签个数（个/张）" << "有效数量（张数）" << "总数");

    totalItem = new QTableWidgetItem("0");
    totalItem->setTextAlignment(Qt::AlignCenter);
    tabelWidget->setItem(0, 2, totalItem);
    tabelWidget->setSpan(0, 2, 6, 1); // 总数跨越6行1列

    // 初始化表格内容
    QStringList labels = {"1-5", "6-10", "11-15", "16-20", "21-25", "25以上"};
    for (int i = 0; i < 6; ++i) {
        tabelWidget->setItem(i, 0, new QTableWidgetItem(labels[i]));
        tabelWidget->setItem(i, 1, new QTableWidgetItem("0"));
        tabelWidget->item(i,0)->setTextAlignment(Qt::AlignCenter); // (可选) 文本居中
        tabelWidget->item(i,1)->setTextAlignment(Qt::AlignCenter); // (可选) 文本居中
    }
    tabelWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 列宽自适应

    labelTabelWidget = new QTableWidget(0, 2, centralWidget); // 初始行数为0，动态添加
    labelTabelWidget->setHorizontalHeaderLabels(QStringList() << "标签名称" << "个数");
    labelTabelWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    mainLayout->addWidget(tabelWidget);
    mainLayout->addWidget(labelTabelWidget);

    centralWidget->setLayout(mainLayout);
    this->setCentralWidget(centralWidget);

    // 初始时禁用分析按钮，因为尚未加载XML
    btnAnalyze->setEnabled(false);
    btnAnalyzeBox->setEnabled(false);
}


void MainWindow::connect_all()
{
    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::handleLoadXml);
    connect(btnAnalyze, &QPushButton::clicked, this, &MainWindow::handleAnalyzeDistribution);
    connect(btnAnalyzeBox, &QPushButton::clicked, this, &MainWindow::handleAnalyzeBoxCounts);
}

void MainWindow::handleLoadXml()
{
    // 记录上次选择的目录，方便用户再次选择
    QString lastDir = xml_dir_.isEmpty() ? QDir::homePath() : xml_dir_;
    xml_dir_ = QFileDialog::getExistingDirectory(this, "选择XML文件夹", lastDir);
    if (xml_dir_.isEmpty()) {
        qDebug() << "未选择目录。";
        btnAnalyze->setEnabled(false);
        btnAnalyzeBox->setEnabled(false);
        return;
    }

    labelDir->setText("当前选择目录 : " + xml_dir_);

    xml_list_.clear(); // 清除之前的结果

    QStringList nameFilters;
    nameFilters << "*.xml";
    QDirIterator it(xml_dir_,
                    nameFilters,
                    QDir::Files | QDir::Readable, // 只查找文件，可读
                    QDirIterator::Subdirectories); // 包含子目录

    while (it.hasNext()) {
        QString filePath = it.next();
        xml_list_.push_back(filePath);
    }

    if (xml_list_.isEmpty()) {
        QMessageBox::information(this, "提示", "选择的文件夹中没有找到XML文件。");
        btnAnalyze->setEnabled(false);
        btnAnalyzeBox->setEnabled(false);
    } else {
        QMessageBox::information(this, "提示", QString("加载了 %1 个XML文件。").arg(xml_list_.size()));
        btnAnalyze->setEnabled(true); // 加载成功后启用分析按钮
        btnAnalyzeBox->setEnabled(true);
    }
}

void MainWindow::handleAnalyzeDistribution()
{
    if (xml_list_.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先加载XML文件。");
        return;
    }
    qDebug() << "主线程: 请求处理XML分布，文件数：" << xml_list_.size();
    emit requestXmlDistributionProcessing(xml_list_);
}

void MainWindow::handleAnalyzeBoxCounts()
{
    if (xml_list_.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先加载XML文件。");
        return;
    }
    qDebug() << "主线程: 请求处理XML标签个数，文件数：" << xml_list_.size();
    emit requestXmlBoxCountProcessing(xml_list_);
}


void MainWindow::updateDistributionTable(const QVector<int>& counts, int totalFilesProcessed)
{
    if (counts.size() != 6) {
        qWarning() << "收到的分布表计数值大小异常:" << counts.size();
        return;
    }

    for(int i = 0; i < 6; i++)
    {
        // 确保 item(i,1) 存在
        if (tabelWidget->item(i, 1)) {
            tabelWidget->item(i,1)->setText(QString::number(counts[i]));
        } else {
            QTableWidgetItem* newItem = new QTableWidgetItem(QString::number(counts[i]));
            newItem->setTextAlignment(Qt::AlignCenter);
            tabelWidget->setItem(i, 1, newItem);
        }
    }
    // 更新总数
    if (tabelWidget->item(0, 2)) {
        tabelWidget->item(0,2)->setText(QString::number(totalFilesProcessed)); // 使用工作线程返回的总数
    }
}

void MainWindow::updateBoxCountTable(const QMap<QString, int>& boxMap)
{
    labelTabelWidget->setRowCount(0); // 清除之前的条目

    for (QMap<QString, int>::const_iterator it = boxMap.constBegin(); it != boxMap.constEnd(); ++it) {
        int new_row = labelTabelWidget->rowCount();
        labelTabelWidget->insertRow(new_row);
        QTableWidgetItem* keyItem = new QTableWidgetItem(it.key());
        QTableWidgetItem* valueItem = new QTableWidgetItem(QString::number(it.value()));
        keyItem->setTextAlignment(Qt::AlignCenter);
        valueItem->setTextAlignment(Qt::AlignCenter);
        labelTabelWidget->setItem(new_row, 0, keyItem);
        labelTabelWidget->setItem(new_row, 1, valueItem);
    }
    if (boxMap.isEmpty()) {
        // 如果没有标签，可以考虑显示一个提示，或者保持表格为空
        labelTabelWidget->insertRow(0);
        labelTabelWidget->setItem(0, 0, new QTableWidgetItem("没有找到标签"));
        labelTabelWidget->setSpan(0,0,1,2); // 跨列显示
    }
}

void MainWindow::onProcessingStarted() {
    btnAnalyze->setEnabled(false);
    btnAnalyzeBox->setEnabled(false);
    btnLoad->setEnabled(false); // 处理期间也禁用加载按钮
}

void MainWindow::onProcessingFinished() {
    // 根据XML是否已加载来重新启用分析按钮
    bool xmlsLoaded = !xml_list_.isEmpty();
    btnAnalyze->setEnabled(xmlsLoaded);
    btnAnalyzeBox->setEnabled(xmlsLoaded);
    btnLoad->setEnabled(true); // 加载按钮总是可以重新启用
}


MainWindow::~MainWindow()
{
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit(); // 请求线程的事件循环退出
        if (!workerThread_->wait(5000)) { // 等待最多5秒
            qWarning() << "工作线程未能正常结束，将强制终止...";
            workerThread_->terminate(); // 如果不退出则强制终止
            workerThread_->wait();      // 等待终止完成
        }
    }
    // workerThread_ 由于设置了 this 为父对象，会被 Qt 自动删除（如果 MainWindow 是堆分配的）
    // xmlProcessor_ 由于连接了 workerThread_ 的 finished 信号到 deleteLater，也会被安全删除。
    // 因此，通常不需要在这里显式 delete workerThread_ 或 xmlProcessor_。
    qDebug() << "MainWindow析构: 已清理工作线程。";
}
