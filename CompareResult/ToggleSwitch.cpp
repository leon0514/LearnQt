#include "ToggleSwitch.h"
#include <QResizeEvent> // For resizeEvent type hint

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent), m_currentSliderPositionX(0) // Initialize m_currentSliderPositionX
{
    setCheckable(true); // Make it behave like a toggle button

    m_animation = new QPropertyAnimation(this, "sliderPosition", this);
    m_animation->setDuration(150); // Default animation duration
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);

    // Connect the toggled signal to ensure checkStateSet is called correctly
    // (though checkStateSet is an override and will be called automatically
    // when setChecked or toggle is called)
    // connect(this, &ToggleSwitch::toggled, this, &ToggleSwitch::checkStateSet); // Not strictly needed due to override

    // Initial calculation and state setting
    // Note: width() and height() might not be final here yet.
    // calculateMetrics() will be called reliably in the first paintEvent or resizeEvent.
    // However, to set an initial m_currentSliderPositionX based on default state:
    // We'll do it in a way that checkStateSet will set the correct initial position.
    // Or simpler: initialize m_currentSliderPositionX to what it would be if off.
    // This will be properly set in the first resize/paint.
    // For now, ensure checkStateSet sets the initial non-animated position.
    // Call checkStateSet directly *after* setChecked to set initial state without animation
    // Or better, handle initial state carefully.
    setChecked(false); // Set initial state
    // Ensure slider is at the correct initial position without animation
    // This needs valid metrics, so it's better done after first resize/paint,
    // or calculateMetrics must be callable with potentially zero width/height.
    // Let's rely on resizeEvent/paintEvent and checkStateSet.
    // For the very first display, checkStateSet will handle the initial position.
    // If setChecked is called, checkStateSet runs.
    // If checkStateSet runs, it animates. To avoid initial animation:
    m_currentSliderPositionX = getTargetSliderPosition(); // Set initial position without animation
}

QSize ToggleSwitch::sizeHint() const
{
    // Provide a reasonable default size
    // Height will be based on some arbitrary radius, width will be roughly twice that.
    int h = 2 * m_padding + 20; // e.g., 20 for track height
    int w = 2 * m_padding + 40; // e.g., 40 for track width
    return QSize(w, h);
}

int ToggleSwitch::sliderPosition() const
{
    return m_currentSliderPositionX;
}

void ToggleSwitch::setSliderPosition(int position)
{
    if (m_currentSliderPositionX != position) {
        m_currentSliderPositionX = position;
        update(); // Trigger a repaint when the animated property changes
    }
}

void ToggleSwitch::setOnColor(const QColor &color)
{
    if (m_onColor != color) {
        m_onColor = color;
        update();
    }
}

void ToggleSwitch::setOffColor(const QColor &color)
{
    if (m_offColor != color) {
        m_offColor = color;
        update();
    }
}

void ToggleSwitch::setSliderColor(const QColor &color)
{
    if (m_sliderColor != color) {
        m_sliderColor = color;
        update();
    }
}

void ToggleSwitch::setHoverColorFactor(const QColor &color)
{
    if (m_hoverColorAdjustment != color) {
        m_hoverColorAdjustment = color;
        update();
    }
}

void ToggleSwitch::setAnimationDuration(int ms)
{
    m_animation->setDuration(ms);
}


void ToggleSwitch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    calculateMetrics(); // Ensure metrics are up-to-date with current size

    // --- Determine Colors ---
    QColor trackColor;
    QColor currentSliderColor = m_sliderColor;

    if (isEnabled()) {
        trackColor = isChecked() ? m_onColor : m_offColor;
        if (m_hovered) {
            // Apply hover adjustment by adding components (clamped)
            trackColor.setRed(qBound(0, trackColor.red() + m_hoverColorAdjustment.red(), 255));
            trackColor.setGreen(qBound(0, trackColor.green() + m_hoverColorAdjustment.green(), 255));
            trackColor.setBlue(qBound(0, trackColor.blue() + m_hoverColorAdjustment.blue(), 255));
        }
    } else {
        // Disabled state: make colors duller
        trackColor = isChecked() ? m_onColor.darker(130) : m_offColor.darker(130);
        currentSliderColor = m_sliderColor.darker(110);
    }

    // --- Draw Track ---
    // The track is a rounded rectangle filling most of the widget area
    QRectF trackRect(m_padding, m_padding, width() - 2 * m_padding, m_trackHeight);
    painter.setBrush(trackColor);
    // Corner radius of the track is half its height (which is also the slider radius)
    painter.drawRoundedRect(trackRect, m_sliderRadius, m_sliderRadius);

    // --- Draw Slider ---
    // m_currentSliderPositionX is the X-coordinate of the slider's CENTER
    qreal sliderYCenter = m_padding + m_sliderRadius;
    QRectF sliderRect(m_currentSliderPositionX - m_sliderRadius,
                      sliderYCenter - m_sliderRadius,
                      m_sliderRadius * 2,
                      m_sliderRadius * 2);
    painter.setBrush(currentSliderColor);
    painter.drawEllipse(sliderRect);
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isEnabled()) {
        toggle(); // <<<< KEY: This changes the checked state
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void ToggleSwitch::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event); // event is not used in this simple implementation
    if (isEnabled()) {
        m_hovered = true;
        update(); // Trigger repaint to show hover effect
    }
    QAbstractButton::enterEvent(event); // Call base class
}

void ToggleSwitch::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = false; // Always reset hover, even if disabled (though enter won't set it)
    update();
    QAbstractButton::leaveEvent(event);
}

void ToggleSwitch::resizeEvent(QResizeEvent *event)
{
    QAbstractButton::resizeEvent(event);
    calculateMetrics();
    // When resized, snap the slider to its correct position based on the current state
    // without animation. The animation target also needs to be updated.
    m_animation->stop(); // Stop any ongoing animation
    setSliderPosition(getTargetSliderPosition()); // Snap to new position
    // update() is called by setSliderPosition
}

void ToggleSwitch::checkStateSet()
{
    // ... (calculate metrics if needed) ...

    int targetX = getTargetSliderPosition(); // Gets target based on NEW isChecked() state

    if (m_animation->state() == QAbstractAnimation::Running) {
        m_animation->stop();
    }

    if (m_currentSliderPositionX != targetX && width() > 0) {
        m_animation->setStartValue(m_currentSliderPositionX); // Current visual position
        m_animation->setEndValue(targetX);                   // New target position
        m_animation->start();                                // <<<< KEY: Starts the animation
    } else if (m_currentSliderPositionX != targetX && width() == 0) {
        setSliderPosition(targetX);
    }
    // No QAbstractButton::checkStateSet(); call needed as it's empty.
}

void ToggleSwitch::calculateMetrics()
{
    // Calculate drawing parameters based on the widget's current height
    m_trackHeight = qMax(0.0, height() - 2.0 * m_padding); // Ensure non-negative
    m_sliderRadius = m_trackHeight / 2.0;

    // Note: The actual width of the track visual is width() - 2 * m_padding.
    // The slider moves within this space.
    // m_currentSliderPositionX will be calculated based on these metrics.
}

int ToggleSwitch::getTargetSliderPosition() const
{
    // Calculates the target X-coordinate for the slider's CENTER based on state
    // This must be callable even if metrics haven't been fully set by a visible widget,
    // so it uses m_padding and m_sliderRadius which are updated by calculateMetrics.
    // Ensure calculateMetrics() has been called before this if width/height might have changed.

    int sliderCenterX_Off = m_padding + m_sliderRadius;
    // The rightmost position for the slider center:
    // widget_width - padding_right - slider_radius
    int sliderCenterX_On = width() - m_padding - m_sliderRadius;

    return isChecked() ? sliderCenterX_On : sliderCenterX_Off;
}
