#ifndef IMAGEVIEWWIDGET_HPP
#define IMAGEVIEWWIDGET_HPP

#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QWheelEvent>
#include <QResizeEvent>


class ImageViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewWidget(QWidget* parrent = nullptr);
    ~ImageViewWidget();

    struct DrawingItem {
        QRectF rect;    // Rectangle coordinates in the original image's coordinate system
        QColor color;   // Color to draw this item
        QString label;  // Optional label for this item
    };

    // 加载图片
    bool loadImage(const QString& image_path);
    // 设置pixmap
    void setPixmap(const QPixmap& pixmap);
    // 获取pixmap
    QPixmap pixmap() const;


    void clearDrawingData();
    void addRectanglesToDraw(const QList<QRectF>& rects, const QColor& color, const QList<QString>& labels = {});

    // 放大
    void zoomIn(double factor = 1.1);
    // 缩小
    void zoomOut(double factor = 0.9);
    // 重置
    void resetZoom();
    // 适应窗口
    void fitToWindow();
    // 获取当前缩放比例
    double getScaleFactor() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* evennt) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPixmap m_original_pixmap;
    QPixmap m_scaled_pixmap;
    double  m_scaled_factor;
    bool    m_is_dragging;
    QPoint  m_last_mouse_point;
    QPointF m_image_offset;

    // 新增：存储要在图片上绘制的矩形列表 (坐标为原始图片坐标)
    QList<DrawingItem> m_drawingItems; // Stores all items to be drawn


private:
    void updateScaledPixmap();
    void adjustOffset();



    QPointF widgetToImageCoordinates(const QPoint& widgetPoint) const;
    QPointF imageToWidgetCoordinates(const QPointF& imagePoint) const;
    QPointF getCurrentPixmapTopLeftInWidget() const;


};

#endif // IMAGEVIEWWIDGET_HPP
