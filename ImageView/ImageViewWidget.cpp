#include "ImageViewWidget.hpp"

#include <QPainter>
#include <QDebug>
#include <QGuiApplication>


ImageViewWidget::ImageViewWidget(QWidget* parrent):QWidget(parrent), m_scaled_factor(1.0), m_is_dragging(false)
{

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
    if (!m_selected_image_points.isEmpty()) {
        painter.setPen(QPen(Qt::red, 2)); // Pen for points and polygon outline

        QVector<QPointF> widgetPoints;
        for (const QPointF& imgPoint : m_selected_image_points) {
            QPointF widgetPt = imageToWidgetCoordinates(imgPoint);
            widgetPoints.append(widgetPt);
            painter.drawEllipse(widgetPt, 3, 3); // Draw a small circle for each point
        }

        if (widgetPoints.size() >= 2) {
            // Draw lines connecting the points
            painter.setPen(QPen(Qt::cyan, 1, Qt::DashLine));
            painter.drawPolyline(QPolygonF(widgetPoints));
        }

        if (widgetPoints.size() >= 3) {
            // Draw the filled polygon
            QPolygonF polygon(widgetPoints);
            QColor fillColor = Qt::blue;
            fillColor.setAlpha(100); // Semi-transparent fill
            painter.setBrush(QBrush(fillColor));
            painter.setPen(QPen(Qt::blue, 2)); // Outline for the polygon
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
        if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
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
    if (m_is_dragging && (event->buttons() & Qt::LeftButton))
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

