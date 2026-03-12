cmake_minimum_required(VERSION 3.1)
project(ros_image_pan_zoom_viewer_qt)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Instruct CMake to run moc, uic and rcc automatically when needed
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  image_transport
  cv_bridge
  sensor_msgs
)

find_package(OpenCV REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS
  Core
  Widgets
  Gui
)

catkin_package(
  CATKIN_DEPENDS
    roscpp
    image_transport
    cv_bridge
    sensor_msgs
)

include_directories(
  src
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

set(SOURCES
  src/main.cpp
  src/main_window.cpp
  src/image_widget.cpp
)

set(HEADERS
  src/main_window.h
  src/image_widget.h
)

add_executable(image_viewer_qt ${SOURCES} ${HEADERS})

target_link_libraries(image_viewer_qt
  ${catkin_LIBRARIES}
  ${OpenCV_LIBS}
  Qt5::Core
  Qt5::Widgets
  Qt5::Gui
)

install(TARGETS image_viewer_qt
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY config launch
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
