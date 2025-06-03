#include "mainwindow.h"

#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QRegularExpressionMatch>
#include <QMenu>
#include <QClipboard>
#include <QApplication>


static QVector<QPointF> stringToPointFVector(const QString& inputString) {
    QVector<QPointF> points;
    const QString numRegexPart = "([-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)";
    // This numRegexPart means:
    // (                                      // Start capture group for a number
    //   [-+]?                                // Optional sign + or -
    //   (?:                                  // Non-capturing group for OR condition
    //     [0-9]+\\.?[0-9]*                  //  - Digits, then optional decimal point and optional fractional digits (e.g., "123", "123.", "123.45")
    //     |                                  //  OR
    //     \\.[0-9]+                          //  - Decimal point followed by digits (e.g., ".5")
    //   )
    //   (?:[eE][-+]?[0-9]+)?                 // Optional exponent part (e.g., "e+5", "E-10")
    // )                                      // End capture group

    QRegularExpression pointRegex(QStringLiteral("\\(\\s*%1\\s*,\\s*%1\\s*\\)").arg(numRegexPart));
    // Results in something like: \(\s*([-+]?(?:[0-9]+\.?[0-9]*|\.[0-9]+)(?:[eE][-+]?[0-9]+)?)\s*,\s*([-+]?(?:[0-9]+\.?[0-9]*|\.[0-9]+)(?:[eE][-+]?[0-9]+)?)\s*\)

    QRegularExpressionMatchIterator i = pointRegex.globalMatch(inputString);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        // We expect 3 captures: full match, x, y.
        if (match.hasMatch() && match.capturedTexts().size() >= 3) {
            bool okX, okY;
            // Capture group 1 is x, group 2 is y.
            double x = match.captured(1).toDouble(&okX);
            double y = match.captured(2).toDouble(&okY);

            if (okX && okY) {
                points.append(QPointF(x, y)); // Or points.emplaceBack(x, y); in Qt 5.6+
            } else {
                qWarning() << "Warning: Could not convert pointF part:" << match.captured(0)
                << "X part:" << match.captured(1) << "(ok:" << okX << ")"
                << "Y part:" << match.captured(2) << "(ok:" << okY << ")";
            }
        }
    }
    return points;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->setWindowTitle("Image Viewer Demo");
    this->resize(800, 600);

    centralWidget = new QWidget(this);
    mainLayout    = new QVBoxLayout(centralWidget);

    middleLayout = new QHBoxLayout();
    horizontalSplitter = new QSplitter(centralWidget);
    middleLayout->addWidget(horizontalSplitter);

    imageViewer   = new ImageViewWidget(centralWidget);
    imageViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    fileListWidget = new DynamicElidedListWidget(centralWidget);

    horizontalSplitter->addWidget(imageViewer);
    horizontalSplitter->addWidget(fileListWidget);
    horizontalSplitter->setStretchFactor(0, 6);
    horizontalSplitter->setStretchFactor(1, 1);

    mainLayout->addLayout(middleLayout);

    lineEdit = new QLineEdit(centralWidget);
    mainLayout->addWidget(lineEdit);

    buttonLayout = new QHBoxLayout();
    btnLoad      = new QPushButton("加载图片", centralWidget);
    btnZoomIn    = new QPushButton("放大", centralWidget);
    btnZoomOut   = new QPushButton("缩小", centralWidget);
    btnFit       = new QPushButton("适合窗口", centralWidget);
    btnReset     = new QPushButton("100%", centralWidget);
    btnClear     = new QPushButton("清除", centralWidget);
    btnPaint     = new QPushButton("绘制", centralWidget);
    rectCheck    = new QCheckBox("绘制矩形");

    QList<QPushButton*> buttons;
    buttons << btnLoad << btnZoomIn << btnZoomOut << btnFit << btnReset << btnClear << btnPaint;

    space = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnLoad);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnZoomIn);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnZoomOut);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnFit);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnReset);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnClear);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(btnPaint);
    buttonLayout->addItem(space);
    buttonLayout->addWidget(rectCheck);
    buttonLayout->addItem(space);
    mainLayout->addLayout(buttonLayout);

    centralWidget->setLayout(mainLayout);
    this->setCentralWidget(centralWidget);

    int maxWidth = 0;
    int maxHeight = 0;

    for (QPushButton *button : std::as_const(buttons)) {
        if (button) {
            QSize hint = button->sizeHint(); // 获取按钮的建议大小
            maxWidth = std::max(maxWidth, hint.width());
            maxHeight = std::max(maxHeight, hint.height());
        }
    }

    // 可选：增加一些padding
    maxWidth += 10; // 例如，给宽度增加10像素的padding
    // maxHeight += 5; // 例如，给高度增加5像素的padding

    for (QPushButton *button : std::as_const(buttons)) {
        if (button) {
            button->setFixedSize(maxWidth, maxHeight);
            // 或者，如果你只想统一宽度，高度自适应:
            // button->setFixedWidth(maxWidth);
            // 或者，如果你只想统一高度，宽度自适应:
            // button->setFixedHeight(maxHeight);
            // 或者，设置为最小尺寸:
            // button->setMinimumSize(maxWidth, maxHeight);
        }
    }


    QObject::connect(fileListWidget,&QListWidget::doubleClicked,[=](){
        QListWidgetItem *selectedItem = fileListWidget->currentItem();
        if (selectedItem) {
            // 执行你的操作
            QString fullPath = selectedItem->data(Qt::UserRole).toString();
            imageViewer->loadImage(fullPath);
        }
    });

    QObject::connect(btnLoad, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        "Open Image",
                                                        "", //起始目录
                                                        "Image Files (*.png *.jpg *.bmp *.jpeg *.gif)");
        if (!fileName.isEmpty()) {
            if(imageViewer->loadImage(fileName))
            {
                QFontMetrics fm(fileListWidget->font());
                // 您需要估算或获取列表项的可用宽度。
                // fileListWidget->viewport()->width() 是一个起点，但可能需要减去边距、滚动条等。
                // 为了简单起见，这里用一个固定值，或者您可以根据窗口大小动态调整。
                int availableWidth = fileListWidget->viewport()->width() - 20; // 减去一些边距/滚动条宽度
                if (availableWidth <= 50) availableWidth = 200; // 设置一个最小合理宽度

                // Qt::ElideMiddle 看起来通常最好 ("C:/.../file.txt")
                // Qt::ElideRight  ("C:/Users/Us...")
                // Qt::ElideLeft   (".../Folder/file.txt")
                QString elidedPath = fm.elidedText(fileName, Qt::ElideMiddle, availableWidth);

                QListWidgetItem *item = new QListWidgetItem(elidedPath); // 显示省略后的路径
                item->setToolTip(fileName); // 鼠标悬停时显示完整路径
                item->setData(Qt::UserRole, fileName); // 存储完整路径

                fileListWidget->addItem(item);
            }
        }
    });

    QObject::connect(btnZoomIn, &QPushButton::clicked, imageViewer, [this]() {
        double factor = 1.5;
        imageViewer->zoomIn(factor);
    });

    QObject::connect(btnZoomOut, &QPushButton::clicked, imageViewer, [this]() {
        double factor = 0.6;
        imageViewer->zoomOut(factor);
    });

    QObject::connect(btnFit, &QPushButton::clicked, imageViewer, [this]() {
        imageViewer->fitToWindow();
    });

    QObject::connect(btnReset, &QPushButton::clicked, imageViewer, [this]() {
        imageViewer->resetZoom();
    });

    QObject::connect(btnClear, &QPushButton::clicked, imageViewer, [this]() {
        imageViewer->clearSelectedPoints();
    });

    QObject::connect(btnPaint, &QPushButton::clicked, imageViewer, [this]() {
        QString display_text = lineEdit->text();
        QVector<QPointF> points = stringToPointFVector(display_text);
        imageViewer->setPoints(points);
    });

    QObject::connect(rectCheck, &QCheckBox::checkStateChanged, imageViewer, [this]() {
        imageViewer->set_rectangle_mode();
        imageViewer->clearSelectedPoints();
    });

    QObject::connect(imageViewer, &ImageViewWidget::pointsSelected,  // 发射者对象和信号
            this, &MainWindow::handlePointsSelected);     // 接收者对象和槽函数

}


void MainWindow::handlePointsSelected(const QVector<QPointF>& points)
{
    if (points.isEmpty())
    {
        lineEdit->setText("");
    }
    else
    {
        QStringList point_strings;
        for (const QPointF& point : points) {
            // 将每个点格式化为 (x, y) 字符串，保留两位小数
            point_strings.append(QString("(%1, %2)")
                                    .arg(point.x(), 0, 'f', 0)
                                    .arg(point.y(), 0, 'f', 0));
        }
        QString combined_points_string = point_strings.join(", ");

        // 然后将这个组合后的字符串放到方括号内（如果需要的话）
        QString display_text = QString("[%1]").arg(combined_points_string);
        lineEdit->setText(display_text);
    }
}

MainWindow::~MainWindow()
{
    delete lineEdit;
    delete btnClear;
    delete btnReset;
    delete btnPaint;
    delete btnFit;
    delete btnZoomOut;
    delete btnZoomIn;
    delete btnLoad;
    delete buttonLayout;
    delete imageViewer;
    delete mainLayout;
    delete centralWidget;
}
