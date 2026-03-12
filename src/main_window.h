#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QString>
#include <memory>

#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/Image.h>

#include <opencv2/core.hpp>

class ImageWidget;

/**
 * @brief MainWindow
 *
 * Manages:
 *  - ROS node handle, image_transport subscriber
 *  - Periodic ros::spinOnce() via a QTimer
 *  - Image conversion / depth visualization
 *  - Menu bar with keyboard shortcuts
 *  - Depth pseudo-color toggle
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ros::NodeHandle &nh, QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void spinOnce();

private:
    // ROS callback
    void imageCallback(const sensor_msgs::ImageConstPtr &msg);

    // Convert cv::Mat to QImage (handles colour, mono8, mono16, 32FC1)
    QImage matToQImage(const cv::Mat &mat, const std::string &encoding);

    // Normalise a single-channel float/16-bit mat to 8-bit for display
    cv::Mat normalizeMono(const cv::Mat &mat) const;

    // Build menu bar
    void createMenus();

    // -------------------------------------------------------------------
    ros::NodeHandle                     &m_nh;
    image_transport::ImageTransport      m_it;
    image_transport::Subscriber          m_sub;

    ImageWidget                         *m_imageWidget = nullptr;
    QLabel                              *m_statusLabel  = nullptr;
    QTimer                              *m_spinTimer    = nullptr;

    bool                                 m_depthPseudoColor = true;
    QString                              m_topic;
    uint32_t                             m_frameCount = 0;
};
