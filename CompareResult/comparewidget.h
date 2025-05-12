#ifndef COMPAREWIDGET_H
#define COMPAREWIDGET_H

#include <QWidget>

#include "ImageViewWidget.hpp"
#include "ToggleSwitch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog> // 用于打开文件对话框
#include <QScrollArea> // 可选，如果图片非常大，可以放在滚动区域

#include "vocParser.h"

class CompareWidget : public QWidget
{
    Q_OBJECT

public:
    CompareWidget(QWidget *parent = nullptr);
    ~CompareWidget();

private:
    QWidget *centralWidget = nullptr;
    QVBoxLayout *outerLayout = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    QHBoxLayout *topLayout = nullptr;
    QHBoxLayout *midLayout = nullptr;
    QHBoxLayout *bottomLayout = nullptr;

    QPushButton  *btnLoadImgDir = nullptr;
    QPushButton  *btnLoadGtXmlDir = nullptr;
    QPushButton  *btnLoadDtXmlDir = nullptr;
    QPushButton  *btnCompare = nullptr;
    QCheckBox    *checkBoxShow = nullptr;
    QProgressBar *progressBar = nullptr;

    QPushButton  *btnPre = nullptr;
    QPushButton  *btnNext = nullptr;

    ImageViewWidget *imageViewer1 = nullptr;
    ImageViewWidget *imageViewer2 = nullptr;

private:
    QString image_dir;
    QString gt_xml_dir;
    QString dt_xml_dir;

    QVector<QString> image_list;
    QVector<QString> gt_xml_list;
    QVector<QString> dt_xml_list;
    qint64 current_index = 0;

    bool show_tp = true;

    VocParser parser;


private:
    void compare(QString gt_xml_path, QString dt_xml_path, bool tp=true);

};
#endif // COMPAREWIDGET_H
