#include "ImageViewWidget.hpp"

#include <QPainter>
#include <QDebug>
#include <QGuiApplication>



ImageViewWidget::ImageViewWidget(QWidget* parrent):QWidget(parrent), m_scaled_factor(1.0), m_is_dragging(false), m_is_dragging_point(false), m_dragged_point_index(-1)
{
    setMouseTracking(true);
}

ImageViewWidget::~ImageViewWidget()
{
}

void ImageViewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_scaled_pixmap.isNull()) {
        painter.drawText(rect(), Qt::AlignCenter, tr("No Image Loaded"));
        return;
    }
    int x = (width() - m_scaled_pixmap.width()) / 2 + m_image_offset.x();
    int y = (height() - m_scaled_pixmap.height()) / 2 + m_image_offset.y();

    painter.drawPixmap(x, y, m_scaled_pixmap);

    // Draw selected points and region
    if (!m_selected_image_points.isEmpty())
    {
        const QColor normalPointColor = Qt::red;
        const QColor draggingPointColor = Qt::yellow; // 或者 Qt::green, Qt::magenta 等醒目的颜色
        const QColor polygonLineColor = Qt::cyan;
        const QColor polygonFillColor = QColor(0, 0, 255, 100); // 半透明蓝色
        const QColor polygonOutlineColor = Qt::blue;
        const int normalPointSize = 3;
        const int draggingPointSize = 5; // 让拖动的点更大更醒目 (可选)

        painter.setPen(QPen(Qt::red, 2)); // Pen for points and polygon outline
        QVector<QPointF> widgetPoints;
        for (int i = 0; i < m_selected_image_points.size(); ++i) {
            const QPointF& imgPoint = m_selected_image_points[i];
            QPointF widgetPt = imageToWidgetCoordinates(imgPoint);
            widgetPoints.append(widgetPt);

            // 检查当前点是否是正在被拖动的点
            bool isBeingDragged = m_is_dragging_point && (i == m_dragged_point_index);

            // 根据是否在拖动选择颜色和大小
            QColor currentPointColor = isBeingDragged ? draggingPointColor : normalPointColor;
            int currentPointSize = isBeingDragged ? draggingPointSize : normalPointSize;

            painter.setPen(QPen(currentPointColor, 2)); // 设置点的轮廓颜色和宽度
            painter.setBrush(QBrush(currentPointColor)); // 设置点的填充颜色
            painter.drawEllipse(widgetPt, currentPointSize, currentPointSize); // 绘制点
        }

        // --- 绘制连接线 ---
        if (widgetPoints.size() >= 2) {
            painter.setPen(QPen(polygonLineColor, 1, Qt::DashLine)); // 重置画笔用于画线
            painter.setBrush(Qt::NoBrush); // 线条不需要填充
            painter.drawPolyline(QPolygonF(widgetPoints));
        }

        // --- 绘制多边形 ---
        if (widgetPoints.size() >= 3) {
            QPolygonF polygon(widgetPoints);
            painter.setBrush(QBrush(polygonFillColor)); // 设置填充
            painter.setPen(QPen(polygonOutlineColor, 2)); // 设置轮廓线
            painter.drawPolygon(polygon);
        }
    }
}

void ImageViewWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if (!m_original_pixmap.isNull()) {
        updateScaledPixmap();
        adjustOffset();
        update();
    }
}

void ImageViewWidget::wheelEvent(QWheelEvent *event)
{
    if (m_original_pixmap.isNull()) {
        event->ignore();
        return;
    }

    int angleDelta = event->angleDelta().y();
    QPointF mousePosInWidget = event->position();
    QPointF imageTopLeftInWidget((width() - m_scaled_pixmap.width()) / 2.0 + m_image_offset.x(),
                                 (height() - m_scaled_pixmap.height()) / 2.0 + m_image_offset.y());
    QPointF mousePosOnImage = (mousePosInWidget - imageTopLeftInWidget) / m_scaled_factor;


    if (angleDelta > 0) {
        zoomIn();
    } else if (angleDelta < 0) {
        zoomOut();
    }

    QPointF newImageTopLeftInWidget = mousePosInWidget - (mousePosOnImage * m_scaled_factor);
    m_image_offset.setX(newImageTopLeftInWidget.x() - (width() - m_original_pixmap.width() * m_scaled_factor) / 2.0);
    m_image_offset.setY(newImageTopLeftInWidget.y() - (height() - m_original_pixmap.height() * m_scaled_factor) / 2.0);


    adjustOffset();
    updateScaledPixmap();
    update();
    event->accept();
}

void ImageViewWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_is_dragging_point = false; // 重置点拖动状态
        m_dragged_point_index = -1;

        // --- 新增：检查是否点击到某个点 ---
        QPoint clickPos = event->pos();
        for (int i = 0; i < m_selected_image_points.size(); ++i) {
            QPointF pointInWidget = imageToWidgetCoordinates(m_selected_image_points[i]);
            if (!pointInWidget.isNull()) {
                // 计算点击位置和点在窗口坐标系下的距离
                QPointF delta = clickPos - pointInWidget;
                double distance = std::sqrt(QPointF::dotProduct(delta, delta));

                if (distance <= m_hit_radius_pixels) {
                    // 命中了点！开始拖动这个点
                    m_is_dragging_point = true;
                    m_dragged_point_index = i;
                    m_last_mouse_point = clickPos; // 记录起始点，也可用于计算位移
                    setCursor(Qt::SizeAllCursor);  // 设置拖动点的光标样式
                    update(); // 可能需要更新视觉效果（例如高亮被选中的点）
                    event->accept();
                    return; // 找到了要拖动的点，处理完毕
                }
            }
        }
        // --- 结束新增 ---

        if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier && !m_draw_rectangle)
        {
            QPointF imagePoint = widgetToImageCoordinates(event->pos());
            if (!imagePoint.isNull())
            {
                m_selected_image_points.append(imagePoint);
                emit pointsSelected(m_selected_image_points);
                update();
            }
            event->accept();
        }
        else if(QGuiApplication::keyboardModifiers() & Qt::ControlModifier && m_draw_rectangle)
        {
            QPointF imagePoint = widgetToImageCoordinates(event->pos());
            if (!imagePoint.isNull())
            {
                if (m_selected_image_points.size() == 0)
                {
                    m_selected_image_points.append(imagePoint);
                    emit pointsSelected(m_selected_image_points);
                    update();
                }
                else
                {
                    auto left_top_point = m_selected_image_points[0];
                    clearSelectedPoints();
                    float x0 = left_top_point.x();
                    float y0 = left_top_point.y();
                    float x1 = imagePoint.x();
                    float y1 = imagePoint.y();
                    m_selected_image_points.append(left_top_point);
                    m_selected_image_points.append(QPointF(x1, y0));
                    m_selected_image_points.append(QPointF(x1, y1));
                    m_selected_image_points.append(QPointF(x0, y1));
                    QVector<QPointF>().swap(m_rectangle_points);
                    m_rectangle_points.append(left_top_point);
                    m_rectangle_points.append(QPointF(x1, y1));
                    emit pointsSelected(m_rectangle_points);
                    update();
                }
            }
            event->accept();
        }
        else
        {
            // 普通左键点击：开始拖动
            m_is_dragging = true;
            m_last_mouse_point = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (m_selected_image_points.size() > 0)
        {
            m_selected_image_points.pop_back();
            emit pointsSelected(m_selected_image_points);
            update();
        }
        event->accept();
    }
    else {
        event->ignore();
    }
}

void ImageViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    // --- 新增：处理点的拖动 ---
    if (m_is_dragging_point && (event->buttons() & Qt::LeftButton))
    {
        if (m_dragged_point_index >= 0 && m_dragged_point_index < m_selected_image_points.size())
        {
            // 将当前鼠标位置（窗口坐标）转换为图像坐标
            QPointF newImagePoint = widgetToImageCoordinates(event->pos());

            if (!newImagePoint.isNull()) {
                // 更新被拖动点在图像坐标系中的位置
                m_selected_image_points[m_dragged_point_index] = newImagePoint;

                // 不需要 m_last_mouse_point 更新，因为我们直接用当前鼠标位置转换
                // m_last_mouse_point = event->pos(); // 如果需要基于delta移动则取消注释

                update(); // 触发重绘以显示点的新位置
                emit pointsSelected(m_selected_image_points); // 实时发送信号（可选）
                event->accept();
                return; // 点拖动事件已处理
            }
        }
    }
    // --- 结束新增 ---
    else if (m_is_dragging && (event->buttons() & Qt::LeftButton))
    {
        QPoint current_mouse_pos = event->pos();
        QPointF delta = current_mouse_pos - m_last_mouse_point;
        m_image_offset += delta;

        m_last_mouse_point = current_mouse_pos;

        adjustOffset();
        update();
        event->accept();
    } else {
        event->ignore();
    }
}

void ImageViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_is_dragging)
    {
        m_is_dragging = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

bool ImageViewWidget::loadImage(const QString &image_path)
{
    QPixmap new_pixmap(image_path);
    if(new_pixmap.isNull())
    {
        qWarning() << "Failed to load image:" << image_path;
        m_original_pixmap = QPixmap();
        m_scaled_pixmap = QPixmap();
        m_scaled_factor = 1.0;
        m_image_offset = QPointF(0,0);
        update();
        return false;
    }
    setPixmap(new_pixmap);
    return true;
}

QPixmap ImageViewWidget::pixmap() const
{
    return m_original_pixmap;
}

void ImageViewWidget::setPixmap(const QPixmap &pixmap)
{
    m_original_pixmap = pixmap;
    m_scaled_pixmap = m_original_pixmap.scaled(
        m_original_pixmap.size() * m_scaled_factor,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_image_offset = QPointF(0, 0);
    fitToWindow();
}

double ImageViewWidget::getScaleFactor() const
{
    return m_scaled_factor;
}

void ImageViewWidget::fitToWindow()
{
    if (m_original_pixmap.isNull() || width() == 0 || height() == 0) {
        m_scaled_factor = 1.0;
        updateScaledPixmap();
        update();
        return;
    }

    double wRatio = (double)width() / m_original_pixmap.width();
    double hRatio = (double)height() / m_original_pixmap.height();
    m_scaled_factor = qMin(wRatio, hRatio);
    m_image_offset = QPointF(0,0);
    updateScaledPixmap();
    update();
}

void ImageViewWidget::zoomIn(double factor)
{
    if (m_original_pixmap.isNull())
    {
        return;
    }
    m_scaled_factor *= factor;
    updateScaledPixmap();
    update();
}


void ImageViewWidget::zoomOut(double factor)
{
    if (m_original_pixmap.isNull()) return;
    m_scaled_factor *= factor;
    if (m_scaled_factor < 0.01) m_scaled_factor = 0.01;
    updateScaledPixmap();
    update();
}

void ImageViewWidget::resetZoom()
{
    if (m_original_pixmap.isNull()) return;
    m_scaled_factor = 1.0;
    m_image_offset = QPointF(0,0);
    updateScaledPixmap();
    update();
}


void ImageViewWidget::set_rectangle_mode()
{
    m_draw_rectangle = !m_draw_rectangle;
}


void ImageViewWidget::updateScaledPixmap()
{
    if (m_original_pixmap.isNull())
    {
        if (!m_scaled_pixmap.isNull())
        {
            m_scaled_pixmap = QPixmap();
        }
        return;
    }

    if (m_scaled_factor <= 0) {
        qWarning() << "updateScaledPixmap: Invalid scale factor " << m_scaled_factor << ". Using 1.0 instead.";
        m_scaled_factor = 1.0; // Or some other sensible default / minimum
    }
    // 根据 m_scaleFactor 缩放原始图片
    // 使用 Qt::SmoothTransformation 来获得更好的缩放质量
    m_scaled_pixmap = m_original_pixmap.scaled(
        m_original_pixmap.size() * m_scaled_factor,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );
    adjustOffset();
}

void ImageViewWidget::adjustOffset()
{
    if (m_scaled_pixmap.isNull()) return;

    QRectF imageRectInWidget(
        (width() - m_scaled_pixmap.width()) / 2.0 + m_image_offset.x(),
        (height() - m_scaled_pixmap.height()) / 2.0 + m_image_offset.y(),
        m_scaled_pixmap.width(),
        m_scaled_pixmap.height()
        );

    QRectF widgetRect(0,0, width(), height());


    if (m_scaled_pixmap.width() < width())
    {
        double allowableXOffset = (width() - m_scaled_pixmap.width()) / 2.0;
        m_image_offset.setX(qBound(-allowableXOffset, m_image_offset.x(), allowableXOffset));
    }
    else
    {
        double minXOffset = width() - m_scaled_pixmap.width() - (width() - m_scaled_pixmap.width()) / 2.0;
        double maxXOffset = -(width() - m_scaled_pixmap.width()) / 2.0;
        m_image_offset.setX(qBound(minXOffset, m_image_offset.x(), maxXOffset));
    }

    if (m_scaled_pixmap.height() < height())
    {
        double allowableYOffset = (height() - m_scaled_pixmap.height()) / 2.0;
        m_image_offset.setY(qBound(-allowableYOffset, m_image_offset.y(), allowableYOffset));
    }
    else
    {
        double minYOffset = height() - m_scaled_pixmap.height() - (height() - m_scaled_pixmap.height()) / 2.0;
        double maxYOffset = -(height() - m_scaled_pixmap.height()) / 2.0;
        m_image_offset.setY(qBound(minYOffset, m_image_offset.y(), maxYOffset));
    }
}

void ImageViewWidget::clearSelectedPoints()
{
    QVector<QPointF>().swap(m_selected_image_points);
    emit pointsSelected(m_selected_image_points);
    update();
}

void ImageViewWidget::setPoints(const QVector<QPointF>& points)
{
    QVector<QPointF>().swap(m_selected_image_points);
    for(const auto& p : points)
    {
        m_selected_image_points.push_back(p);
    }
    update();
}

QPointF ImageViewWidget::getCurrentPixmapTopLeftInWidget() const
{
    if (m_scaled_pixmap.isNull()) {
        return QPointF(0,0); // Or some other appropriate default
    }
    return QPointF(
        (width() - m_scaled_pixmap.width()) / 2.0 + m_image_offset.x(),
        (height() - m_scaled_pixmap.height()) / 2.0 + m_image_offset.y()
        );
}

QPointF ImageViewWidget::widgetToImageCoordinates(const QPoint& widgetPoint) const
{
    if (m_scaled_pixmap.isNull() || m_scaled_factor == 0) {
        return QPointF(); // Invalid
    }
    QPointF imageTopLeftInWidget = getCurrentPixmapTopLeftInWidget();
    return (QPointF(widgetPoint) - imageTopLeftInWidget) / m_scaled_factor;
}

QPointF ImageViewWidget::imageToWidgetCoordinates(const QPointF& imagePoint) const
{
    if (m_scaled_pixmap.isNull()) {
        return QPointF(); // Invalid
    }
    QPointF imageTopLeftInWidget = getCurrentPixmapTopLeftInWidget();
    return (imagePoint * m_scaled_factor) + imageTopLeftInWidget;
}

