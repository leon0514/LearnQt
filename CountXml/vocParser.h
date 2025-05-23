#ifndef VOCPARSER_H
#define VOCPARSER_H

#include <QString>
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QRect>

struct VocObject
{
    QString name;
    QRect   bndbox;
};

class VocParser
{
public:
    VocParser(){}

    QList<VocObject> parseObjects(const QString& filePath);

private:
    // 辅助函数，用于获取指定标签名下的文本内容
    QString getElementText(const QDomElement& parentElement, const QString& tagName)
    {
        QDomElement element = parentElement.firstChildElement(tagName);
        if (!element.isNull()) {
            return element.text().trimmed();
        }
        return QString();
    }
    // 辅助函数，用于获取指定标签名下的整数内容
    int getElementInt(const QDomElement& parentElement, const QString& tagName)
    {
        QString text = getElementText(parentElement, tagName);
        bool ok;
        int value = text.toInt(&ok);
        return ok ? value : 0; // 可以考虑对转换失败做更明确的处理
    }

    float getElementFloat(const QDomElement& parentElement, const QString& tagName)
    {
        QString text = getElementText(parentElement, tagName);
        bool ok;
        float value = text.toFloat(&ok);
        return ok ? value : 0; // 可以考虑对转换失败做更明确的处理
    }
};

#endif // VOCPARSER_H
