#include "DynamicElidedListWidget.h"
#include <QScrollBar> // To account for scrollbar width
#include <QDebug>     // For optional debugging output
#include <QMimeData> // <<--- 添加这个头文件
#include <QUrl>      // <<--- 添加这个头文件
#include <QFileInfo>

DynamicElidedListWidget::DynamicElidedListWidget(QWidget *parent)
    : QListWidget(parent)
{
    // It's often best to let the text elide rather than show a horizontal scrollbar
    // if the primary goal is to fit text within the current view.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Ensure items are laid out correctly initially (especially if items added before show)
    // However, the first real update will happen on the first resizeEvent or showEvent.
}

QListWidgetItem* DynamicElidedListWidget::addItemWithFullText(const QString &fullText, const QString &initialDisplaySuffix)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setData(Qt::UserRole, fullText); // Store the full, unelided text
    item->setToolTip(fullText);

    // Set an initial text. It will be properly elided by updateAllItemDisplayTexts.
    // Adding a suffix can be useful if the full text is very long and you want a hint.
    if (!initialDisplaySuffix.isEmpty()) {
        item->setText(fullText.left(30) + "..." + initialDisplaySuffix); // Example initial text
    } else {
        item->setText(fullText); // Will be elided on next update pass
    }

    addItem(item);

    // If the widget is already visible and has a valid size, update immediately.
    // Otherwise, resizeEvent will handle it.
    if (isVisible() && !viewport()->size().isEmpty()) {
        // We only need to update this new item if others are already updated
        // For simplicity here, or if called after initial setup, refresh all.
        // A more optimized version might just update the new item.
        // updateSingleItemDisplayText(item); // More optimized version
        refreshItemTexts(); // For simplicity, refresh all
    }
    return item;
}

void DynamicElidedListWidget::refreshItemTexts()
{
    updateAllItemDisplayTexts();
}


void DynamicElidedListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem *item = itemAt(event->pos()); // 获取鼠标位置处的 item

    if (item) { // 确保右键点击在了某个 item 上
        QMenu contextMenu(this); // 创建菜单，父对象是 this (DynamicElidedListWidget)

        // 添加 "复制完整路径" 动作
        QAction *copyFullPathAction = contextMenu.addAction(tr("复制完整路径"));

        // 当 "复制" 动作被触发时，执行 Lambda 表达式中的代码
        // Lambda 捕获被点击的 item
        connect(copyFullPathAction, &QAction::triggered, [item]() {
            QString textToCopy;
            // 优先从 Qt::UserRole 获取完整路径
            QVariant userData = item->data(Qt::UserRole);
            if (userData.isValid() && userData.typeId() == QMetaType::QString) {
                textToCopy = userData.toString();
            } else {
                // 如果 UserRole 没有有效数据，则回退到 item 的显示文本 (虽然通常不希望这样)
                // 对于这个特定功能，我们主要关心完整路径
                qWarning() << "Copy Full Path: Item does not have valid UserRole data (full path).";
                // textToCopy = item->text(); // 可以选择是否回退
            }

            if (!textToCopy.isEmpty()) {
                QClipboard *clipboard = QApplication::clipboard();
                clipboard->setText(textToCopy);
                qDebug() << "Copied to clipboard (from DynamicElidedListWidget):" << textToCopy;
            }
        });

        // (可选) 添加其他动作，例如 "复制显示文本"
        QAction *copyDisplayTextAction = contextMenu.addAction(tr("复制显示文本"));
        connect(copyDisplayTextAction, &QAction::triggered, [item](){
            QApplication::clipboard()->setText(item->text());
        });

        contextMenu.addSeparator(); // 添加分隔符

        // --- 3. 复制文件 (到系统剪贴板，用于粘贴到文件管理器) ---
        QAction *copyFileAction = contextMenu.addAction(tr("复制文件"));
        // 只有当 fullPath 是一个有效的文件路径时才启用此操作
        // (可选的额外检查：确保文件实际存在)
        QString fullPath = item->data(Qt::UserRole).toString();
        QFileInfo fileInfo(fullPath);
        if (fullPath.isEmpty() || !fileInfo.isFile()) { // 检查是否是文件
            copyFileAction->setEnabled(false);
            copyFileAction->setToolTip(tr("Not a valid file or path is empty."));
        }

        connect(copyFileAction, &QAction::triggered, [fullPath]() { // 捕获 fullPath
            if (fullPath.isEmpty()) return;

            QList<QUrl> urls;
            urls.append(QUrl::fromLocalFile(fullPath)); // 将本地文件路径转换为 URL

            if (urls.isEmpty()) {
                qWarning() << "Could not create URL from path:" << fullPath;
                return;
            }

            QMimeData *mimeData = new QMimeData();
            mimeData->setUrls(urls); // 设置 URL 列表

            // (可选) 为了更好的兼容性，可以同时设置纯文本路径
            // mimeData->setText(fullPath);

            QApplication::clipboard()->setMimeData(mimeData);
        });

        // 在鼠标的全局位置显示菜单
        // event->globalPos() 直接提供了全局坐标
        contextMenu.exec(event->globalPos());
    } else {
        // 如果没有点击在 item 上 (例如点击在空白区域)，
        // 可以选择调用基类的 contextMenuEvent，如果 QListWidget 有默认行为的话。
        // QListWidget::contextMenuEvent(event);
        // 或者什么都不做
    }
}

void DynamicElidedListWidget::resizeEvent(QResizeEvent *event)
{
    QListWidget::resizeEvent(event); // Call base class implementation first
    // qDebug() << "DynamicElidedListWidget resized, new viewport width:" << viewport()->width();
    updateAllItemDisplayTexts();     // Update texts based on new size
}

/*
// Optional: Handle font changes explicitly if needed,
// though resizeEvent often triggers if font change affects widget's overall size.
void DynamicElidedListWidget::changeEvent(QEvent* event)
{
    QListWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange) {
        updateAllItemDisplayTexts();
    }
}
*/

int DynamicElidedListWidget::estimateAvailableTextWidth() const
{
    if (!viewport()) return 50; // Fallback if viewport not ready

    int availableWidth = viewport()->width();

    // Account for vertical scrollbar if visible
    QScrollBar *vScroll = verticalScrollBar();
    if (vScroll && vScroll->isVisible()) {
        availableWidth -= vScroll->width();
    }

    // Account for internal item margins, icon space, etc.
    availableWidth -= m_itemHorizontalMargin;

    // Ensure a minimum positive width to avoid issues with QFontMetrics
    return qMax(20, availableWidth);
}

QString DynamicElidedListWidget::calculateElidedTextForItem(const QString& fullText, int availableWidth) const
{
    if (fullText.isEmpty()) {
        return QString();
    }
    QFontMetrics fm(this->font()); // Use the list widget's current font
    return fm.elidedText(fullText, Qt::ElideMiddle, availableWidth); // ElideMiddle is good for paths
}

void DynamicElidedListWidget::updateAllItemDisplayTexts()
{
    if (!isVisible() || viewport()->width() <= 0) {
        // Don't do expensive updates if not visible or not yet sized
        return;
    }

    int textWidth = estimateAvailableTextWidth();
    // qDebug() << "Updating items with available text width:" << textWidth;

    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *listItem = item(i);
        if (listItem) {
            QString fullText = listItem->data(Qt::UserRole).toString();
            if (!fullText.isEmpty()) {
                QString elidedText = calculateElidedTextForItem(fullText, textWidth);
                // Only set text if it actually changes, to avoid unnecessary redraws/signals
                if (listItem->text() != elidedText) {
                    listItem->setText(elidedText);
                }
            } else {
                // Handle items that might not have full text in UserRole, or clear them
                if (!listItem->text().isEmpty()) {
                    listItem->setText(QString());
                }
            }
        }
    }
}
