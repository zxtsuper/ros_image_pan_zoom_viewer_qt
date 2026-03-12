#pragma once

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QPointF>

/**
 * @brief ImageWidget
 *
 * Handles rendering, zooming, panning, mouse interaction, overlay drawing,
 * and screenshot saving for a single ROS-sourced image.
 *
 * View modes
 * ----------
 * - FitToWindow  : image is scaled to fill the widget while preserving aspect ratio
 * - ActualSize   : image is displayed at 1:1 pixel ratio
 * - Free         : user-controlled zoom / pan
 *
 * Zoom modifiers (mouse wheel)
 * ----------------------------
 * - No modifier : ~1.2x per step
 * - Shift       : fine zoom ~1.08x per step
 * - Ctrl        : fast zoom ~1.5x per step
 */
class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    enum class ViewMode { FitToWindow, ActualSize, Free };

    explicit ImageWidget(QWidget *parent = nullptr);

    void setImage(const QImage &image);

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return m_viewMode; }

    void fitToWindow();
    void actualSize();
    void resetView();

    void setShowCrosshair(bool show);
    bool showCrosshair() const { return m_showCrosshair; }

    bool saveScreenshot(const QString &filePath);

    double zoomFactor() const { return m_zoom; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // Convert widget coordinates to image pixel coordinates
    QPointF widgetToImage(const QPoint &widgetPos) const;

    // Zoom around a widget-space anchor point
    void zoomAround(const QPoint &anchor, double factor);

    // Clamp offset so image stays at least partially visible
    void clampOffset();

    QImage   m_image;
    double   m_zoom          = 1.0;
    QPointF  m_offset;          // top-left corner of the image in widget coords
    ViewMode m_viewMode       = ViewMode::FitToWindow;
    bool     m_showCrosshair  = false;

    bool     m_dragging       = false;
    QPoint   m_lastMousePos;

    QPoint   m_mousePos;        // last known mouse position in widget coords
};
