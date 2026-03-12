#include "main_window.h"
#include "image_widget.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QStatusBar>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>

#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc.hpp>

namespace enc = sensor_msgs::image_encodings;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
MainWindow::MainWindow(ros::NodeHandle &nh, QWidget *parent)
    : QMainWindow(parent)
    , m_nh(nh)
    , m_it(nh)
{
    // Resolve topic from private param ~topic (default: /camera/image_raw)
    ros::NodeHandle pnh("~");
    std::string topicStd = "/camera/image_raw";
    pnh.param<std::string>("topic", topicStd, topicStd);
    m_topic = QString::fromStdString(topicStd);

    setWindowTitle(QString("ROS Image Viewer  [%1]").arg(m_topic));
    resize(1024, 768);

    // Central widget
    m_imageWidget = new ImageWidget(this);
    setCentralWidget(m_imageWidget);

    // Status bar
    m_statusLabel = new QLabel("Waiting for images…", this);
    statusBar()->addWidget(m_statusLabel, 1);

    createMenus();

    // Subscribe
    m_sub = m_it.subscribe(topicStd, 1, &MainWindow::imageCallback, this);

    // Spin timer – 30 ms ≈ 33 Hz
    m_spinTimer = new QTimer(this);
    connect(m_spinTimer, &QTimer::timeout, this, &MainWindow::spinOnce);
    m_spinTimer->start(30);
}

// ---------------------------------------------------------------------------
// Menu / shortcuts
// ---------------------------------------------------------------------------
void MainWindow::createMenus()
{
    // --- View menu ----------------------------------------------------------
    QMenu *viewMenu = menuBar()->addMenu("&View");

    QAction *fitAct = new QAction("&Fit to window", this);
    fitAct->setShortcut(QKeySequence("F"));
    connect(fitAct, &QAction::triggered, this, [this]() {
        m_imageWidget->setViewMode(ImageWidget::ViewMode::FitToWindow);
    });
    viewMenu->addAction(fitAct);

    QAction *actualAct = new QAction("&Actual size (1:1)", this);
    actualAct->setShortcut(QKeySequence("1"));
    connect(actualAct, &QAction::triggered, this, [this]() {
        m_imageWidget->setViewMode(ImageWidget::ViewMode::ActualSize);
    });
    viewMenu->addAction(actualAct);

    QAction *resetAct = new QAction("&Reset view", this);
    resetAct->setShortcut(QKeySequence("R"));
    connect(resetAct, &QAction::triggered, m_imageWidget, &ImageWidget::resetView);
    viewMenu->addAction(resetAct);

    viewMenu->addSeparator();

    QAction *crosshairAct = new QAction("Toggle &crosshair", this);
    crosshairAct->setShortcut(QKeySequence("C"));
    crosshairAct->setCheckable(true);
    connect(crosshairAct, &QAction::toggled, m_imageWidget, &ImageWidget::setShowCrosshair);
    viewMenu->addAction(crosshairAct);

    QAction *depthAct = new QAction("Toggle &depth pseudo-color", this);
    depthAct->setShortcut(QKeySequence("D"));
    depthAct->setCheckable(true);
    depthAct->setChecked(m_depthPseudoColor);
    connect(depthAct, &QAction::toggled, this, [this](bool checked) {
        m_depthPseudoColor = checked;
    });
    viewMenu->addAction(depthAct);

    // --- File menu ----------------------------------------------------------
    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *saveAct = new QAction("&Save screenshot…", this);
    saveAct->setShortcut(QKeySequence("S"));
    connect(saveAct, &QAction::triggered, this, [this]() {
        QString ts   = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QString path = QFileDialog::getSaveFileName(
            this, "Save screenshot",
            QString("screenshot_%1.png").arg(ts),
            "Images (*.png *.jpg *.bmp)");
        if (path.isEmpty())
            return;
        if (!m_imageWidget->saveScreenshot(path))
            QMessageBox::warning(this, "Save failed",
                                 "Could not save screenshot to:\n" + path);
    });
    fileMenu->addAction(saveAct);

    fileMenu->addSeparator();

    QAction *quitAct = new QAction("&Quit", this);
    quitAct->setShortcut(QKeySequence("Escape"));
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(quitAct);
}

// ---------------------------------------------------------------------------
// ROS spin slot
// ---------------------------------------------------------------------------
void MainWindow::spinOnce()
{
    if (!ros::ok())
        qApp->quit();
    ros::spinOnce();
}

// ---------------------------------------------------------------------------
// Image callback
// ---------------------------------------------------------------------------
void MainWindow::imageCallback(const sensor_msgs::ImageConstPtr &msg)
{
    ++m_frameCount;
    try {
        cv::Mat mat;
        std::string encoding = msg->encoding;

        // Depth / mono 16-bit and 32-bit handling
        bool isDepth = (encoding == enc::MONO16 ||
                        encoding == "16UC1"      ||
                        encoding == "TYPE_16UC1" ||
                        encoding == "32FC1"      ||
                        encoding == "TYPE_32FC1");

        if (isDepth) {
            cv_bridge::CvImageConstPtr cvPtr =
                cv_bridge::toCvShare(msg, encoding);
            mat = normalizeMono(cvPtr->image);

            if (m_depthPseudoColor && mat.type() == CV_8UC1) {
                cv::Mat colorMat;
                cv::applyColorMap(mat, colorMat, cv::COLORMAP_JET);
                cv::cvtColor(colorMat, colorMat, cv::COLOR_BGR2RGB);
                mat = colorMat;
            }
        } else {
            // Colour or mono8
            cv_bridge::CvImageConstPtr cvPtr;
            if (encoding == enc::MONO8 || encoding == "mono8") {
                cvPtr = cv_bridge::toCvShare(msg, enc::MONO8);
            } else {
                cvPtr = cv_bridge::toCvShare(msg, enc::RGB8);
            }
            mat = cvPtr->image.clone();
        }

        QImage qimg = matToQImage(mat, mat.channels() == 1 ? "mono8" : "rgb8");
        if (!qimg.isNull()) {
            m_imageWidget->setImage(qimg);
            m_statusLabel->setText(
                QString("Topic: %1  |  Frame: %2  |  Size: %3×%4  |  Enc: %5")
                    .arg(m_topic)
                    .arg(m_frameCount)
                    .arg(msg->width)
                    .arg(msg->height)
                    .arg(QString::fromStdString(encoding)));
        }
    } catch (const cv_bridge::Exception &e) {
        ROS_WARN_STREAM("cv_bridge exception: " << e.what());
    } catch (const std::exception &e) {
        ROS_WARN_STREAM("imageCallback exception: " << e.what());
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
cv::Mat MainWindow::normalizeMono(const cv::Mat &src) const
{
    cv::Mat dst;
    if (src.type() == CV_32FC1) {
        cv::Mat tmp = src.clone();
        // Replace NaN / Inf with 0 before normalizing
        cv::patchNaNs(tmp, 0.0);
        cv::normalize(tmp, tmp, 0, 255, cv::NORM_MINMAX, CV_32FC1);
        tmp.convertTo(dst, CV_8UC1);
    } else {
        // 16-bit
        double minVal, maxVal;
        cv::minMaxLoc(src, &minVal, &maxVal);
        if (maxVal > minVal)
            src.convertTo(dst, CV_8UC1, 255.0 / (maxVal - minVal),
                          -minVal * 255.0 / (maxVal - minVal));
        else
            src.convertTo(dst, CV_8UC1);
    }
    return dst;
}

QImage MainWindow::matToQImage(const cv::Mat &mat, const std::string &encoding)
{
    if (mat.empty())
        return QImage();

    if (encoding == "mono8" || mat.channels() == 1) {
        // Grayscale
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step),
                      QImage::Format_Grayscale8).copy();
    } else if (mat.channels() == 3) {
        // RGB
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step),
                      QImage::Format_RGB888).copy();
    } else if (mat.channels() == 4) {
        // RGBA
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step),
                      QImage::Format_RGBA8888).copy();
    }
    return QImage();
}
