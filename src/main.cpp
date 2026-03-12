#include <QApplication>
#include <ros/ros.h>
#include "main_window.h"

int main(int argc, char *argv[])
{
    // Qt must be initialised before ros::init so that the display connection
    // is available when ROS sets up signal handling.
    QApplication app(argc, argv);

    ros::init(argc, argv, "image_viewer_qt",
              ros::init_options::NoSigintHandler);

    if (!ros::master::check()) {
        qCritical("ROS master is not running. Please start roscore first.");
        return 1;
    }

    ros::NodeHandle nh;

    MainWindow window(nh);
    window.show();

    return app.exec();
}
