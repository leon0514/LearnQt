#include "vocParser.h"


QList<VocObject> VocParser::parseObjects(const QString& filePath)
{
    QList<VocObject> objectsList;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Error: Cannot open XML file:" << filePath << file.errorString();
        return objectsList; // 返回空列表
    }

    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn))
    {
        qWarning() << "Error: Failed to parse XML file:" << filePath
                   << "Reason:" << errorMsg << "at line" << errorLine << "column" << errorColumn;
        file.close();
        return objectsList; // 返回空列表
    }
    file.close();

    QDomElement root = doc.documentElement(); // <annotation>
    if (root.tagName() != "annotation") {
        qWarning() << "Error: XML root element is not <annotation> in file:" << filePath;
        return objectsList; // 返回空列表
    }

    QDomNodeList objectNodes = root.elementsByTagName("object");
    for (int i = 0; i < objectNodes.count(); ++i) {
        QDomElement objectElement = objectNodes.at(i).toElement();
        if (objectElement.isNull()) continue;

        VocObject obj;
        obj.name = getElementText(objectElement, "name");

        QDomElement bndboxElement = objectElement.firstChildElement("bndbox");
        if (!bndboxElement.isNull()) {
            int xmin = (int)getElementFloat(bndboxElement, "xmin");
            int ymin = (int)getElementFloat(bndboxElement, "ymin");
            int xmax = (int)getElementFloat(bndboxElement, "xmax");
            int ymax = (int)getElementFloat(bndboxElement, "ymax");

            if (xmax < xmin || ymax < ymin) { // 基本的有效性检查
                qWarning() << "Warning: Invalid bndbox coordinates (xmax < xmin or ymax < ymin) in" << filePath << "for object" << obj.name;
                obj.bndbox = QRect(); // 设置为无效矩形
            } else {
                // QRect 构造函数是 (left, top, width, height)
                obj.bndbox = QRect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
            }
        } else {
            qWarning() << "Warning: <bndbox> not found for an object named '" << obj.name << "' in" << filePath;
            obj.bndbox = QRect(); // 无效矩形
        }

        // 只有当 name 和 bndbox 都有效时才添加 (可选，根据需求)
        if (!obj.name.isEmpty() && obj.bndbox.isValid() && obj.bndbox.width() > 0 && obj.bndbox.height() > 0) {
            objectsList.append(obj);
        } else if (!obj.name.isEmpty()) {
            // 如果name有效但bndbox无效，你可能还是想记录这个物体，只是框是无效的
            // objectsList.append(obj); // 取决于你的需求
            qWarning() << "Warning: Object '" << obj.name << "' in" << filePath << " has an invalid or missing bndbox.";
        }
    }

    return objectsList;
}
