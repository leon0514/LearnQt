#ifndef XMLPROCESSOR_H
#define XMLPROCESSOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include "vocParser.h" // 确保这个路径是正确的

class XmlProcessor : public QObject
{
    Q_OBJECT
public:
    explicit XmlProcessor(QObject *parent = nullptr);

public slots:
    // 处理XML标签分布的槽函数
    void processXmlDistribution(const QVector<QString>& xmlFiles);
    // 处理XML标签个数统计的槽函数
    void processXmlBoxCounts(const QVector<QString>& xmlFiles);

signals:
    // 标签分布处理完成信号，参数为各区间的数量和处理的总文件数
    void distributionProcessingFinished(const QVector<int>& counts, int totalFilesProcessed);
    // 标签个数统计完成信号，参数为标签名和对应的数量
    void boxCountProcessingFinished(const QMap<QString, int>& boxMap);
    // 可选：开始处理信号，用于禁用UI元素
    void processingStarted();
    // 可选：处理结束信号，用于重新启用UI元素
    void processingFinished();

private:
    VocParser parser_; // VocParser 实例，在工作线程中使用
};


#endif // XMLPROCESSOR_H
