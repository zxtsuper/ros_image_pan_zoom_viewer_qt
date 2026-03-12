#include "image_widget.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QFileDialog>
#include <QPixmap>
#include <QtMath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void ImageWidget::setImage(const QImage &image)
{
    m_image = image;

    if (m_viewMode == ViewMode::FitToWindow)
        fitToWindow();
    else if (m_viewMode == ViewMode::ActualSize)
        actualSize();

    update();
}

void ImageWidget::setViewMode(ViewMode mode)
{
    m_viewMode = mode;
    if (mode == ViewMode::FitToWindow)
        fitToWindow();
    else if (mode == ViewMode::ActualSize)
        actualSize();
    update();
}

void ImageWidget::fitToWindow()
{
    if (m_image.isNull() || width() == 0 || height() == 0)
        return;

    double scaleX = static_cast<double>(width())  / m_image.width();
    double scaleY = static_cast<double>(height()) / m_image.height();
    m_zoom = std::min(scaleX, scaleY);

    // Centre the image
    m_offset.setX((width()  - m_image.width()  * m_zoom) / 2.0);
    m_offset.setY((height() - m_image.height() * m_zoom) / 2.0);
}

void ImageWidget::actualSize()
{
    if (m_image.isNull())
        return;

    m_zoom = 1.0;
    m_offset.setX((width()  - m_image.width())  / 2.0);
    m_offset.setY((height() - m_image.height()) / 2.0);
}

void ImageWidget::resetView()
{
    m_viewMode = ViewMode::FitToWindow;
    fitToWindow();
    update();
}

void ImageWidget::setShowCrosshair(bool show)
{
    m_showCrosshair = show;
    update();
}

bool ImageWidget::saveScreenshot(const QString &filePath)
{
    if (m_image.isNull())
        return false;

    QPixmap pm(size());
    render(&pm);
    return pm.save(filePath);
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
void ImageWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_zoom < 1.0);
    painter.fillRect(rect(), Qt::darkGray);

    if (!m_image.isNull()) {
        int drawW = static_cast<int>(m_image.width()  * m_zoom);
        int drawH = static_cast<int>(m_image.height() * m_zoom);
        painter.drawImage(
            QRect(static_cast<int>(m_offset.x()),
                  static_cast<int>(m_offset.y()),
                  drawW, drawH),
            m_image);
    }

    // Crosshair overlay
    if (m_showCrosshair) {
        painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
        int cx = width()  / 2;
        int cy = height() / 2;
        painter.drawLine(cx, 0, cx, height());
        painter.drawLine(0, cy, width(), cy);
    }

    // Status overlay: coordinates and pixel value
    if (!m_image.isNull()) {
        QPointF imgPos = widgetToImage(m_mousePos);
        int px = static_cast<int>(imgPos.x());
        int py = static_cast<int>(imgPos.y());

        QString info;
        if (px >= 0 && py >= 0 && px < m_image.width() && py < m_image.height()) {
            QRgb pixel = m_image.pixel(px, py);
            QString zoomStr = QString::number(m_zoom, 'f', 2);
            if (m_image.isGrayscale()) {
                info = QString("(%1, %2)  val=%3  zoom=%4×")
                           .arg(px).arg(py)
                           .arg(qRed(pixel))
                           .arg(zoomStr);
            } else {
                info = QString("(%1, %2)  R=%3 G=%4 B=%5  zoom=%6×")
                           .arg(px).arg(py)
                           .arg(qRed(pixel)).arg(qGreen(pixel)).arg(qBlue(pixel))
                           .arg(zoomStr);
            }
        } else {
            info = QString("zoom=%1×").arg(QString::number(m_zoom, 'f', 2));
        }

        painter.setPen(Qt::white);
        painter.setFont(QFont("Monospace", 9));
        QRect textRect = rect().adjusted(4, 4, -4, -4);
        painter.drawText(textRect, Qt::AlignBottom | Qt::AlignLeft, info);
    }
}

void ImageWidget::resizeEvent(QResizeEvent * /*event*/)
{
    if (m_viewMode == ViewMode::FitToWindow)
        fitToWindow();
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    // Determine zoom factor based on modifier keys
    double factor = 1.2;
    if (event->modifiers() & Qt::ShiftModifier)
        factor = 1.08;
    else if (event->modifiers() & Qt::ControlModifier)
        factor = 1.5;

    int delta = event->angleDelta().y();
    if (delta == 0)
        return;

    double zoomFactor = (delta > 0) ? factor : (1.0 / factor);

    m_viewMode = ViewMode::Free;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    zoomAround(event->position().toPoint(), zoomFactor);
#else
    zoomAround(event->pos(), zoomFactor);
#endif
    update();
    event->accept();
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging    = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->pos();

    if (m_dragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_offset += delta;
        m_viewMode = ViewMode::Free;
        clampOffset();
    }

    update();
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
QPointF ImageWidget::widgetToImage(const QPoint &widgetPos) const
{
    if (m_zoom == 0.0)
        return QPointF(-1, -1);
    double ix = (widgetPos.x() - m_offset.x()) / m_zoom;
    double iy = (widgetPos.y() - m_offset.y()) / m_zoom;
    return QPointF(ix, iy);
}

void ImageWidget::zoomAround(const QPoint &anchor, double factor)
{
    // image coordinate that is under the anchor before zoom
    QPointF imgAnchor = widgetToImage(anchor);

    m_zoom *= factor;
    // clamp zoom to a sensible range (1/32 .. 64)
    m_zoom = std::max(1.0 / 32.0, std::min(64.0, m_zoom));

    // Recompute offset so that imgAnchor stays under the anchor
    m_offset.setX(anchor.x() - imgAnchor.x() * m_zoom);
    m_offset.setY(anchor.y() - imgAnchor.y() * m_zoom);

    clampOffset();
}

void ImageWidget::clampOffset()
{
    if (m_image.isNull())
        return;

    double imgW = m_image.width()  * m_zoom;
    double imgH = m_image.height() * m_zoom;

    // At minimum keep at least 1 pixel of the image inside the widget
    double minX = -imgW + 1;
    double maxX =  width() - 1;
    double minY = -imgH + 1;
    double maxY =  height() - 1;

    m_offset.setX(std::max(minX, std::min(maxX, m_offset.x())));
    m_offset.setY(std::max(minY, std::min(maxY, m_offset.y())));
}
