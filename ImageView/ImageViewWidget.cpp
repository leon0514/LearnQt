#include "ImageViewWidget.hpp"

#include <QPainter>
#include <QDebug>
#include <QGuiApplication>


static bool isPointInImageBounds(const QPointF& imagePoint, const QPixmap& originalPixmap) {
    if (originalPixmap.isNull()) {
        return false;
    }
    return imagePoint.x() >= 0 && imagePoint.x() < originalPixmap.width() &&
           imagePoint.y() >= 0 && imagePoint.y() < originalPixmap.height();
}

// 将点限制在图像边界内（图像坐标系）
static QPointF clampPointToImageBounds(const QPointF& imagePoint, const QPixmap& originalPixmap) {
    if (originalPixmap.isNull()) {
        return imagePoint; // 或者返回一个无效点 QPointF()
    }
    qreal clampedX = qBound(0.0, imagePoint.x(), qreal(originalPixmap.width() - 1));  // 减1是因为像素坐标从0开始
    qreal clampedY = qBound(0.0, imagePoint.y(), qreal(originalPixmap.height() - 1)); // 减1
    return QPointF(clampedX, clampedY);
}

ImageViewWidget::ImageViewWidget(QWidget* parrent):QWidget(parrent), m_scaled_factor(1.0), m_is_dragging(false), m_is_dragging_point(false), m_dragged_point_index(-1), m_is_dragging_polygon(false)
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
    // --- 定义点的颜色和大小 ---
    const QColor firstPointColor = Qt::green;       // 起始点颜色
    const QColor lastPointColor = Qt::white;         // 结束点颜色
    const QColor intermediatePointColor = Qt::red;  // 中间点颜色
    const QColor draggingPointColor = Qt::yellow;   // 正在拖动的点颜色
    const QColor polygonLineColor = Qt::cyan;
    const QColor polygonFillColor = QColor(0, 0, 255, 100); // 半透明蓝色
    const QColor polygonOutlineColor = Qt::blue;

    const int normalPointSize = 4;        // 普通点大小
    const int specialPointSize = 5;       // 首尾点或拖动点的大小（可以比普通点稍大）
    const int draggingPointSize = 6;      // 拖动点可以更大更醒目


    QVector<QPointF> widgetPoints;
    for (int i = 0; i < m_selected_image_points.size(); ++i) {
        const QPointF& imgPoint = m_selected_image_points[i];
        QPointF widgetPt = imageToWidgetCoordinates(imgPoint);
        widgetPoints.append(widgetPt);

        QColor currentPointColor;
        int currentPointSize = normalPointSize;

        bool isBeingDragged = m_is_dragging_point && (i == m_dragged_point_index);

        if (isBeingDragged) {
            currentPointColor = draggingPointColor;
            currentPointSize = draggingPointSize;
        } else if (i == 0) { // 第一个点
            currentPointColor = firstPointColor;
            currentPointSize = specialPointSize;
        } else if (i == m_selected_image_points.size() - 1 && m_selected_image_points.size() > 1) { // 最后一个点 (且不止一个点时)
            currentPointColor = lastPointColor;
            currentPointSize = specialPointSize;
        } else { // 中间点
            currentPointColor = intermediatePointColor;
            currentPointSize = normalPointSize;
        }

        // 如果只有一个点，它既是首也是尾，按首点颜色处理 (已被 i == 0 覆盖)
        // 如果你希望只有一个点时有特殊颜色，可以添加条件：
        // if (m_selected_image_points.size() == 1) {
        // currentPointColor = Qt::magenta; // Example: single point color
        // currentPointSize = specialPointSize;
        // }


        painter.setPen(QPen(currentPointColor, 2)); // 点的轮廓颜色和宽度
        painter.setBrush(QBrush(currentPointColor));   // 点的填充颜色
        painter.drawEllipse(widgetPt, currentPointSize, currentPointSize); // 绘制点
    }

    // --- 绘制连接线 ---
    // 只有当点的数量大于等于2时才绘制连接线
    if (widgetPoints.size() >= 2) {
        painter.setPen(QPen(polygonLineColor, 1, Qt::DashLine)); // 重置画笔用于画线
        painter.setBrush(Qt::NoBrush); // 线条不需要填充
        painter.drawPolyline(QPolygonF(widgetPoints));
    }

    // --- 绘制多边形 ---
    // 只有当点的数量大于等于3时才绘制填充的多边形
    if (widgetPoints.size() >= 3) {
        QPolygonF polygon(widgetPoints);
        painter.setBrush(QBrush(polygonFillColor)); // 设置填充
        painter.setPen(QPen(polygonOutlineColor, 2)); // 设置轮廓线
        painter.drawPolygon(polygon);
    }
}

void ImageViewWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if (!m_original_pixmap.isNull()) {
        updateScaledPixmap();
        adjustOffset();
        update();
        fitToWindow();
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
    if (event->button() == Qt::LeftButton && !QGuiApplication::keyboardModifiers())
    {
        m_is_dragging_point = false; // 重置点拖动状态
        m_is_dragging_polygon = false;
        m_dragged_point_index = -1;

        QPoint clickPosWidget = event->pos();
        QPointF clickPosImage = widgetToImageCoordinates(clickPosWidget);

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

        if (m_selected_image_points.size() >= 3) // 通常四边形是4个点
        {
            QPolygonF polygonInImageCoords(m_selected_image_points);
            if (polygonInImageCoords.containsPoint(clickPosImage, Qt::OddEvenFill)) // Qt::OddEvenFill 是常用填充规则
            {
                m_is_dragging_polygon = true;
                m_last_mouse_point = clickPosWidget; // 记录拖动起始的窗口坐标
                    // 或者 m_drag_start_image_pos = clickPosImage;
                setCursor(Qt::SizeAllCursor); // 或者 Qt::OpenHandCursor -> Qt::ClosedHandCursor
                update(); // 可能需要更新视觉效果
                event->accept();
                return;
            }
        }
        m_is_dragging = true;
        m_last_mouse_point = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
    else if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier && !m_draw_rectangle)
    {
        QPointF imagePoint = widgetToImageCoordinates(event->pos());
        if (!imagePoint.isNull())
        {
            if (isPointInImageBounds(imagePoint, m_original_pixmap))
            {
                m_selected_image_points.append(imagePoint);
                emit pointsSelected(m_selected_image_points);
                update();
            }
        }
        event->accept();
    }
    else if(QGuiApplication::keyboardModifiers() & Qt::ControlModifier && m_draw_rectangle)
    {
        QPointF imagePoint = widgetToImageCoordinates(event->pos());
        if (!imagePoint.isNull())
        {
            QPointF clampedImagePoint = clampPointToImageBounds(imagePoint, m_original_pixmap);
            if (m_selected_image_points.size() == 0)
            {
                m_selected_image_points.append(clampedImagePoint);
                emit pointsSelected(m_selected_image_points);
                update();
            }
            else
            {
                auto left_top_point = m_selected_image_points[0];
                clearSelectedPoints();
                float x0 = left_top_point.x();
                float y0 = left_top_point.y();
                float x1 = clampedImagePoint.x();
                float y1 = clampedImagePoint.y();
                m_selected_image_points.append(left_top_point);
                m_selected_image_points.append(QPointF(x1, y0));
                m_selected_image_points.append(QPointF(x1, y1));
                m_selected_image_points.append(QPointF(x0, y1));
                QVector<QPointF>().swap(m_rectangle_points);
                m_rectangle_points.append(left_top_point);
                m_rectangle_points.append(clampedImagePoint);
                emit pointsSelected(m_rectangle_points);
                update();
            }
        }
        event->accept();
    }
    else if (event->button() == Qt::RightButton)
    {
        if (m_selected_image_points.size() > 0)
        {
            m_selected_image_points.pop_back();
            if (m_draw_rectangle && m_selected_image_points.size() > 0)
            {
                m_selected_image_points.pop_back();
                m_selected_image_points.pop_back();
            }
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
    if (m_original_pixmap.isNull())
    {
        if (m_is_dragging && (event->buttons() & Qt::LeftButton))
        {
            // 允许平移空画布
            QPoint current_mouse_pos = event->pos();
            QPointF delta = current_mouse_pos - m_last_mouse_point;
            m_image_offset += delta;
            m_last_mouse_point = current_mouse_pos;
            adjustOffset(); // adjustOffset 会处理 m_scaled_pixmap.isNull()
            update();
            event->accept();
        }
        else
        {
            event->ignore();
        }
        return;
    }

    QPoint currentMouseWidgetPos = event->pos();
    if (m_is_dragging_point && (event->buttons() & Qt::LeftButton))
    {
        if (m_dragged_point_index >= 0 && m_dragged_point_index < m_selected_image_points.size())
        {
            // 将当前鼠标位置（窗口坐标）转换为图像坐标
            QPointF newImagePoint = widgetToImageCoordinates(event->pos());

            if (!newImagePoint.isNull())
            {
                QPointF clampedNewImagePoint = clampPointToImageBounds(newImagePoint, m_original_pixmap);
                m_selected_image_points[m_dragged_point_index] = clampedNewImagePoint;
                if (!m_draw_rectangle)
                {
                    update(); // 触发重绘以显示点的新位置
                    emit pointsSelected(m_selected_image_points); // 实时发送信号（可选）
                    event->accept();
                    return; // 点拖动事件已处理
                }
                else
                {
                    int opposite_point_index = (m_dragged_point_index + 2) % 4;
                    QPointF oppositePoint = m_selected_image_points[opposite_point_index];

                    float new_x0 = qMin(clampedNewImagePoint.x(), oppositePoint.x());
                    float new_y0 = qMin(clampedNewImagePoint.y(), oppositePoint.y());
                    float new_x1 = qMax(clampedNewImagePoint.x(), oppositePoint.x());
                    float new_y1 = qMax(clampedNewImagePoint.y(), oppositePoint.y());
                    clearSelectedPoints();
                    m_selected_image_points.append(QPointF(new_x0, new_y0)); // 新的左上
                    m_selected_image_points.append(QPointF(new_x1, new_y0)); // 新的右上
                    m_selected_image_points.append(QPointF(new_x1, new_y1)); // 新的右下
                    m_selected_image_points.append(QPointF(new_x0, new_y1)); // 新的左下
                    QVector<QPointF>().swap(m_rectangle_points);
                    m_rectangle_points.append(QPointF(new_x0, new_y0));
                    m_rectangle_points.append(QPointF(new_x1, new_y1));
                    emit pointsSelected(m_rectangle_points);
                    update();
                }
            }
        }
    }
    else if (m_is_dragging_polygon && (event->buttons() & Qt::LeftButton))
    {
        QPointF lastMouseImagePos = widgetToImageCoordinates(m_last_mouse_point);
        QPointF currentMouseImagePos = widgetToImageCoordinates(currentMouseWidgetPos);

        if (!lastMouseImagePos.isNull() && !currentMouseImagePos.isNull())
        {
            QPointF deltaImage = currentMouseImagePos - lastMouseImagePos; // 计算在图像坐标系下的位移

            // 检查平移后是否所有点都仍在界内 (可选，或者在平移后clamp)
            bool all_points_will_be_in_bounds = true;
            for (const QPointF& pt : m_selected_image_points) {
                if (!isPointInImageBounds(pt + deltaImage, m_original_pixmap)) {
                    all_points_will_be_in_bounds = false;
                    break;
                }
            }

            if (all_points_will_be_in_bounds)
            {
                for (int i = 0; i < m_selected_image_points.size(); ++i)
                {
                    m_selected_image_points[i] += deltaImage;
                }
            }
            m_last_mouse_point = currentMouseWidgetPos; // 更新上一次的鼠标位置 (窗口坐标)
            update();
            if (m_draw_rectangle)
            {
                auto left_top_point = m_rectangle_points[0] + deltaImage;
                auto right_bottom_point = m_rectangle_points[1] + deltaImage;
                clearSelectedPoints();
                float x0 = left_top_point.x();
                float y0 = left_top_point.y();
                float x1 = right_bottom_point.x();
                float y1 = right_bottom_point.y();
                m_selected_image_points.append(left_top_point);
                m_selected_image_points.append(QPointF(x1, y0));
                m_selected_image_points.append(QPointF(x1, y1));
                m_selected_image_points.append(QPointF(x0, y1));
                QVector<QPointF>().swap(m_rectangle_points);
                m_rectangle_points.append(left_top_point);
                m_rectangle_points.append(right_bottom_point);
                emit pointsSelected(m_rectangle_points);
                update();
            }
            else
            {
                emit pointsSelected(m_selected_image_points); // 发送更新
            }

            event->accept();
            return;
        }
    }
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
    if (event->button() == Qt::LeftButton)
    {
        bool was_any_drag_active = false;

        if (m_is_dragging)
        {
            m_is_dragging = false;
            was_any_drag_active = true;
        }
        if (m_is_dragging_point)
        {
            m_is_dragging_point = false;
            // m_dragged_point_index = -1; // Already reset at the start of mousePressEvent,
            // but good practice to ensure it's invalid when not dragging a point.
            // Let's ensure it's reset here too for clarity.
            m_dragged_point_index = -1;
            was_any_drag_active = true;
        }
        if (m_is_dragging_polygon)
        {
            m_is_dragging_polygon = false;
            was_any_drag_active = true;
        }

        if (was_any_drag_active)
        {
            setCursor(Qt::ArrowCursor); // Reset to default cursor
            event->accept();
        }
        else
        {
            // If no drag was active, it might have been a simple click.
            // Depending on your application's needs, you might accept or ignore.
            // If click-without-drag has no specific action on release, ignoring is fine.
            event->ignore();
        }
    }
    else
    {
        event->ignore();
    }
    update();
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
    if (m_scaled_pixmap.isNull())
    {
        return;
    }

    const qreal view_w = width();
    const qreal view_h = height();
    const qreal img_w = m_scaled_pixmap.width();
    const qreal img_h = m_scaled_pixmap.height();

    const qreal half_delta_w = (view_w - img_w) / 2.0;
    const qreal half_delta_h = (view_h - img_h) / 2.0;

    // 如果 img_w < view_w, half_delta_w > 0. min_offset_x = -half_delta_w, max_offset_x = half_delta_w.
    // 如果 img_w >= view_w, half_delta_w <= 0. min_offset_x = half_delta_w, max_offset_x = -half_delta_w.
    const qreal min_offset_x = qMin(half_delta_w, -half_delta_w);
    const qreal max_offset_x = qMax(half_delta_w, -half_delta_w);
    const qreal min_offset_y = qMin(half_delta_h, -half_delta_h);
    const qreal max_offset_y = qMax(half_delta_h, -half_delta_h);

    m_image_offset.setX(qBound(min_offset_x, m_image_offset.x(), max_offset_x));
    m_image_offset.setY(qBound(min_offset_y, m_image_offset.y(), max_offset_y));
}

void ImageViewWidget::clearSelectedPoints()
{
    QVector<QPointF>().swap(m_selected_image_points);
    emit pointsSelected(m_selected_image_points);
    update();
}

void ImageViewWidget::setPoints(const QVector<QPointF>& points)
{
    m_selected_image_points = points;
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
        return QPointF();
    }
    QPointF imageTopLeftInWidget = getCurrentPixmapTopLeftInWidget();
    return (QPointF(widgetPoint) - imageTopLeftInWidget) / m_scaled_factor;
}

QPointF ImageViewWidget::imageToWidgetCoordinates(const QPointF& imagePoint) const
{
    if (m_scaled_pixmap.isNull()) {
        return QPointF();
    }
    QPointF imageTopLeftInWidget = getCurrentPixmapTopLeftInWidget();
    return (imagePoint * m_scaled_factor) + imageTopLeftInWidget;
}

