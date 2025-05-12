#include "comparewidget.h"
#include <QDirIterator>

static double calculateIoU(const QRect& r1, const QRect& r2)
{
    int xA = std::max(r1.left(), r2.left());
    int yA = std::max(r1.top(), r2.top());
    int xB = std::min(r1.right(), r2.right()); // QRect right() is x + width - 1
    int yB = std::min(r1.bottom(), r2.bottom()); // QRect bottom() is y + height - 1

    int interWidth = std::max(0, xB - xA + 1);
    int interHeight = std::max(0, yB - yA + 1);
    double interArea = static_cast<double>(interWidth * interHeight);

    if (interArea == 0) {
        return 0.0;
    }

    double box1Area = static_cast<double>(r1.width() * r1.height());
    double box2Area = static_cast<double>(r2.width() * r2.height());

    double iou = interArea / (box1Area + box2Area - interArea);

    return iou;
}

void CompareWidget::compare(QString gt_xml_path, QString dt_xml_path, bool tp)
{
    QList<VocObject> gt_obj_list = parser.parseObjects(gt_xml_path);
    QList<VocObject> dt_obj_list = parser.parseObjects(dt_xml_path);

    const double IOU_THRESHOLD = 0.5;
    const QColor TP_COLOR(0, 255, 0, 150);    // Green (True Positive)
    const QColor FP_COLOR(0, 0, 255, 150);    // Blue (False Positive)
    const QColor FN_COLOR(255, 0, 0, 150);    // Red (False Negative)

    QList<QRectF> gt_rects_tp;
    QList<QString> gt_labels_tp;
    QList<QRectF> gt_rects_fn;
    QList<QString> gt_labels_fn;

    // For Detection Viewer (imageViewer2)
    QList<QRectF> dt_rects_tp;
    QList<QString> dt_labels_tp;
    QList<QRectF> dt_rects_fp;
    QList<QString> dt_labels_fp;

    // Keep track of matched detections to avoid matching them multiple times
    QVector<bool> dt_matched_flags(dt_obj_list.size(), false);

    // --- Matching Process ---
    // Iterate through ground truth objects
    for (const VocObject& gt_obj : gt_obj_list)
    {
        double best_iou = 0.0;
        int best_dt_match_idx = -1;

        // Find the best matching detection for the current GT object
        for (int i = 0; i < dt_obj_list.size(); ++i)
        {
            if (dt_matched_flags[i]) // If this detection is already matched to a higher IoU GT
            {
                // Optional: If you want to allow a detection to match multiple GTs (uncommon), remove this.
                // Or, if a GT can only match one DT, and a DT can only match one GT (common for evaluation)
                // this greedy approach might need refinement (e.g. Hungarian algorithm for optimal assignment)
                // For visualization, this greedy approach is often sufficient.
                // continue; // Let's allow a DT to be considered for multiple GTs, but only one GT claims it.
            }

            const VocObject& dt_obj = dt_obj_list[i];

            if (gt_obj.name == dt_obj.name) // Class names must match
            {
                double iou = calculateIoU(gt_obj.bndbox, dt_obj.bndbox);
                if (iou > best_iou)
                {
                    best_iou = iou;
                    best_dt_match_idx = i;
                }
            }
        }

        QRectF gt_bndbox_f(gt_obj.bndbox.left(), gt_obj.bndbox.top(), gt_obj.bndbox.width(), gt_obj.bndbox.height());

        if (best_dt_match_idx != -1 && best_iou >= IOU_THRESHOLD && !dt_matched_flags[best_dt_match_idx])
        {
            // --- True Positive (TP) ---
            // This GT object is matched with a DT object
            gt_rects_tp.append(gt_bndbox_f);
            gt_labels_tp.append(QString("TP: %1 (IoU: %2)").arg(gt_obj.name).arg(best_iou, 0, 'f', 2));

            const VocObject& matched_dt_obj = dt_obj_list[best_dt_match_idx];
            QRectF dt_bndbox_f(matched_dt_obj.bndbox.left(), matched_dt_obj.bndbox.top(), matched_dt_obj.bndbox.width(), matched_dt_obj.bndbox.height());
            dt_rects_tp.append(dt_bndbox_f);
            dt_labels_tp.append(QString("TP: %1 (IoU: %2)").arg(matched_dt_obj.name).arg(best_iou, 0, 'f', 2));

            dt_matched_flags[best_dt_match_idx] = true; // Mark this detection as matched
        }
        else
        {
            // --- False Negative (FN) ---
            // This GT object was not detected or matched with low IoU
            gt_rects_fn.append(gt_bndbox_f);
            gt_labels_fn.append(QString("FN: %1").arg(gt_obj.name));
                // If best_dt_match_idx was valid but IoU too low, that dt_obj is still considered unmatched for now
        }
    }

    // --- Identify False Positives (FP) ---
    // Iterate through detection objects to find those not matched
    for (int i = 0; i < dt_obj_list.size(); ++i)
    {
        if (!dt_matched_flags[i])
        {
            const VocObject& dt_obj = dt_obj_list[i];
            QRectF dt_bndbox_f(dt_obj.bndbox.left(), dt_obj.bndbox.top(), dt_obj.bndbox.width(), dt_obj.bndbox.height());
            dt_rects_fp.append(dt_bndbox_f);
            dt_labels_fp.append(QString("FP: %1").arg(dt_obj.name));
        }
    }

    // --- Update Image Viewers ---
    // This assumes your ImageViewer has methods like clearRectangles() and
    // addRectangles(rects, color, labels)
    // You might need to adapt this part based on your ImageViewer's API.

    imageViewer1->clearDrawingData(); // Hypothetical method
    if (!gt_rects_tp.isEmpty() && tp) {
        imageViewer1->addRectanglesToDraw(gt_rects_tp, TP_COLOR, gt_labels_tp);
    }
    if (!gt_rects_fn.isEmpty()) {
        imageViewer1->addRectanglesToDraw(gt_rects_fn, FN_COLOR, gt_labels_fn);
    }
    imageViewer1->update(); // Trigger repaint

    imageViewer2->clearDrawingData(); // Hypothetical method
    if (!dt_rects_tp.isEmpty() && tp) {
        imageViewer2->addRectanglesToDraw(dt_rects_tp, TP_COLOR, dt_labels_tp);
    }
    if (!dt_rects_fp.isEmpty()) {
        imageViewer2->addRectanglesToDraw(dt_rects_fp, FP_COLOR, dt_labels_fp);
    }
    imageViewer2->update(); // Trigger repaint
}


CompareWidget::CompareWidget(QWidget *parent)
    : QWidget(parent)
{
    outerLayout = new QVBoxLayout(this);

    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    topLayout = new QHBoxLayout();
    midLayout = new QHBoxLayout();
    bottomLayout = new QHBoxLayout();

    btnLoadImgDir   = new QPushButton("图片路径", centralWidget);
    btnLoadGtXmlDir = new QPushButton("GT xml 路径", centralWidget);
    btnLoadDtXmlDir = new QPushButton("DT xml 路径", centralWidget);
    btnCompare      = new QPushButton("对比", centralWidget);
    // btnFilterTp  = new QPushButton("当前显示TP标签", centralWidget);
    checkBoxShow    = new QCheckBox("当前显示TP", centralWidget);
    checkBoxShow->setStyleSheet(
        "QCheckBox::indicator:unchecked {"
        "    background-color: #4CAF50; /* 未选中时的背景色 (不亮 - 浅灰色) */"
        "    border: 1px solid #A0A0A0; /* 未选中时的边框 */"
        "    width: 15px;               /* 指示器宽度 */"
        "    height: 15px;              /* 指示器高度 */"
        "    border-radius: 3px;        /* 可选：圆角 */"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: #E0E0E0; /* 选中时的背景色 (亮 - 绿色) */"
        "    border: 1px solid #A0A0A0; /* 选中时的边框 */"
        "    /* 如果你想用一张图片作为勾选标记，可以这样: */"
        "    /* image: url(:/my_icons/checked_mark.png); */"
        "}"
        "QCheckBox::indicator:unchecked:hover {"
        "    border: 1px solid #707070; /* 鼠标悬停在未选中指示器上 */"
        "}"
        "QCheckBox::indicator:checked:hover {"
        "    border: 1px solid #2E7D32; /* 鼠标悬停在选中指示器上 */"
        "}"
        "QCheckBox {"
        "    spacing: 5px; /* 指示器和文本之间的间距 */"
        "    color: #333333; /* 文本颜色 */"
        "    font-size: 14px;"
        "}"
        "QCheckBox:disabled {" // 可选：禁用时的样式
        "    color: #A0A0A0;"
        "}"
        "QCheckBox::indicator:disabled {" // 可选：禁用时指示器的样式
        "    background-color: #F0F0F0;"
        "    border: 1px solid #C0C0C0;"
        "}"
        );

    progressBar     = new QProgressBar(centralWidget);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 8px;"
        "    background-color: #E0E0E0;" // 背景色
        "    height: 20px;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4CAF50, stop:1 #8BC34A);" // 渐变填充
        "    border-radius: 7px;" // chunk的圆角要比外部小一点，或者不设置
        "}"
    );

    imageViewer1   = new ImageViewWidget(centralWidget);
    imageViewer2   = new ImageViewWidget(centralWidget);
    imageViewer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageViewer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    midLayout->addWidget(imageViewer1);
    midLayout->addWidget(imageViewer2);


    btnPre       = new QPushButton("上一张", centralWidget);
    btnNext      = new QPushButton("下一张", centralWidget);

    topLayout->addWidget(btnLoadImgDir);
    topLayout->addWidget(btnLoadGtXmlDir);
    topLayout->addWidget(btnLoadDtXmlDir);
    topLayout->addWidget(btnCompare);
    topLayout->addWidget(checkBoxShow);
    topLayout->addWidget(progressBar);
    topLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    bottomLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    bottomLayout->addWidget(btnPre);
    bottomLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    bottomLayout->addWidget(btnNext);
    bottomLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(midLayout);
    mainLayout->addLayout(bottomLayout);

    midLayout->setStretchFactor(imageViewer1, 1);
    midLayout->setStretchFactor(imageViewer2, 1);

    outerLayout->addWidget(centralWidget);

    // 槽函数绑定
    QObject::connect(btnLoadImgDir, &QPushButton::clicked, this, [this]() {
        image_dir = QFileDialog::getExistingDirectory();
        if (image_dir.isEmpty()) {
            qDebug() << "No directory selected.";
            return; // User cancelled
        }

        qDebug() << "Selected directory:" << image_dir;

        image_list.clear(); // Clear previous results

        QStringList nameFilters;
        nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
        QDirIterator it(image_dir,
                        nameFilters,
                        QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            QString filePath = it.next(); // it.next() advances and returns the current path
            image_list.push_back(filePath);
        }

        if (image_list.size() > 0)
        {
            progressBar->setRange(0, image_list.size());
            progressBar->setValue(0); // Initial value
            current_index = 0;
            imageViewer1->loadImage(image_list[current_index]);
            imageViewer2->loadImage(image_list[current_index]);
        }
    });

    QObject::connect(btnLoadGtXmlDir, &QPushButton::clicked, this, [this]() {
        gt_xml_dir = QFileDialog::getExistingDirectory();
        if (gt_xml_dir.isEmpty()) {
            qDebug() << "No directory selected.";
            return; // User cancelled
        }

        qDebug() << "Selected directory:" << gt_xml_dir;

        gt_xml_list.clear(); // Clear previous results

        QStringList nameFilters;
        nameFilters << "*.xml";
        QDirIterator it(gt_xml_dir,
                        nameFilters,
                        QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            QString filePath = it.next(); // it.next() advances and returns the current path
            gt_xml_list.push_back(filePath);
        }

    });

    QObject::connect(btnLoadDtXmlDir, &QPushButton::clicked, this, [this]() {
        dt_xml_dir = QFileDialog::getExistingDirectory();
        if (dt_xml_dir.isEmpty()) {
            qDebug() << "No directory selected.";
            return; // User cancelled
        }

        qDebug() << "Selected directory:" << dt_xml_dir;

        dt_xml_list.clear(); // Clear previous results

        QStringList nameFilters;
        nameFilters << "*.xml";
        QDirIterator it(dt_xml_dir,
                        nameFilters,
                        QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            QString filePath = it.next(); // it.next() advances and returns the current path
            dt_xml_list.push_back(filePath);
        }
    });

    QObject::connect(btnCompare, &QPushButton::clicked, this, [this]() {
        if (image_list.size() > current_index && dt_xml_list.size() > 0 && gt_xml_list.size() > 0)
        {
            compare(gt_xml_list[current_index], dt_xml_list[current_index], show_tp);
        }
    });

    QObject::connect(checkBoxShow, &QCheckBox::checkStateChanged, this, [this]() {
        show_tp = !show_tp;
        if (show_tp)
        {
            checkBoxShow->setText("当前显示TP");
        }
        else
        {
            checkBoxShow->setText("当前不显示TP");
        }
        if (image_list.size() > current_index && dt_xml_list.size() > 0 && gt_xml_list.size() > 0)
        {
            compare(gt_xml_list[current_index], dt_xml_list[current_index], show_tp);
        }
    });


    QObject::connect(btnPre, &QPushButton::clicked, this, [this]() {
        if (current_index - 1 >= 0) // Check if a previous item exists
        {
            current_index -= 1; // Update to the new index first

            // Calculate progress based on the new current_index
            if (!image_list.isEmpty()) {
                // (current_index + 1) because it's 1-based for "count" of items processed
                // e.g., if current_index is 0 (first item), progress is (0+1)/total * 100
                double progress_percentage = (static_cast<double>(current_index + 1) / image_list.size()) * 100.0;
                progressBar->setValue(static_cast<int>(progress_percentage));
            } else {
                progressBar->setValue(0); // Or handle as an error/empty state
            }

            imageViewer1->loadImage(image_list[current_index]);
            imageViewer2->loadImage(image_list[current_index]);
            if (image_list.size() > current_index && dt_xml_list.size() > 0 && gt_xml_list.size() > 0)
            {
                compare(gt_xml_list[current_index], dt_xml_list[current_index], show_tp);
            }
        }
    });

    QObject::connect(btnNext, &QPushButton::clicked, this, [this]() {
        // qDebug() << "Before Next, current_index:" << current_index << "image_list.size():" << image_list.size();
        if (current_index + 1 < image_list.size()) // Check if a next item exists
        {
            current_index += 1; // Update to the new index first
            // qDebug() << "After Next, new current_index:" << current_index;

            if (!image_list.isEmpty()) {
                double progress_percentage = (static_cast<double>(current_index + 1) / image_list.size()) * 100.0;
                progressBar->setValue(static_cast<int>(progress_percentage));
            } else {
                progressBar->setValue(0);
            }

            imageViewer1->loadImage(image_list[current_index]);
            imageViewer2->loadImage(image_list[current_index]);
            if (image_list.size() > current_index && dt_xml_list.size() > 0 && gt_xml_list.size() > 0)
            {
                compare(gt_xml_list[current_index], dt_xml_list[current_index], show_tp);
            }
        }
    });

}

CompareWidget::~CompareWidget()
{

}
