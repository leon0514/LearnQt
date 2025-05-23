#include "xmlprocessor.h"
#include <QDebug>
#include <QThread> // 用于调试输出当前线程ID

XmlProcessor::XmlProcessor(QObject *parent) : QObject(parent)
{
}

void XmlProcessor::processXmlDistribution(const QVector<QString>& xmlFiles)
{
    emit processingStarted(); // 发送开始处理信号

    QVector<int> xml_count = {0, 0, 0, 0, 0, 0}; // 局部变量存储统计结果
    int totalValidFiles = 0; // 统计有效文件（包含object的文件）的数量

    for (const QString& filePath : xmlFiles)
    {
        QList<VocObject> obj_list = parser_.parseObjects(filePath);
        int totalObjectsInFile = obj_list.size();
        if (totalObjectsInFile == 0)
        {
            continue; // 如果文件没有object，则跳过
        }
        totalValidFiles++; // 有效文件数增加
        int index = (totalObjectsInFile - 1) / 5;
        if (index > 5) // 最大索引是5 (对应 "25以上")
        {
            index = 5;
        }
        xml_count[index] += 1;
    }
    // 发送处理完成信号，携带统计结果和有效文件总数
    emit distributionProcessingFinished(xml_count, totalValidFiles);
    emit processingFinished(); // 发送处理结束信号
    qDebug() << "工作线程: XML分布处理完成。";
}

void XmlProcessor::processXmlBoxCounts(const QVector<QString>& xmlFiles)
{
    emit processingStarted(); // 发送开始处理信号
    QMap<QString, int> box_map; // 局部变量存储统计结果

    for (const QString& filePath : xmlFiles)
    {
        QList<VocObject> obj_list = parser_.parseObjects(filePath);
        for(const auto& obj : std::as_const(obj_list))
        {
            QString label_name = obj.name;
            box_map[label_name] += 1;
        }
    }
    emit boxCountProcessingFinished(box_map);
    emit processingFinished(); // 发送处理结束信号
}
