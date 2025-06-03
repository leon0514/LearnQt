#ifndef DYNAMICELIDEDLISTWIDGET_H
#define DYNAMICELIDEDLISTWIDGET_H

#include <QListWidget>
#include <QResizeEvent> // For resizeEvent
#include <QFontMetrics> // For elidedText
#include <QStyleOptionViewItem> // For more precise calculations (optional advanced)
#include <QContextMenuEvent>
#include <QMenu>
#include <QClipboard>
#include <QApplication>

class DynamicElidedListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit DynamicElidedListWidget(QWidget *parent = nullptr);

    // Convenience function to add items and store full text in UserRole
    // Automatically triggers an update if the widget is already visible and sized.
    QListWidgetItem* addItemWithFullText(const QString &fullText, const QString &initialDisplaySuffix = QString());

    // Call this if you manually change the font or other style aspects
    // that might affect text rendering width, or if you add many items
    // outside of addItemWithFullText and want to batch update.
    void refreshItemTexts();

protected:
    // Handles widget resize events
    void resizeEvent(QResizeEvent *event) override;

    // Optional: Handles font changes (though resizeEvent usually covers this if font affects sizeHint)
    // void changeEvent(QEvent* event) override;

private:
    // Core logic to update the display text of all items
    void updateAllItemDisplayTexts();

    // Calculates the elided text for a single full text string given available width
    QString calculateElidedTextForItem(const QString& fullText, int availableWidth) const;
    void contextMenuEvent(QContextMenuEvent *event) override;

    // Helper to estimate the available width for text within an item
    int estimateAvailableTextWidth() const;

    // Configuration: estimated horizontal padding/margins/icon space inside an item
    // Adjust this value based on your QListWidget's styling and if items have icons.
    int m_itemHorizontalMargin = 10; // Default, adjust as needed
};

#endif // DYNAMICELIDEDLISTWIDGET_H
