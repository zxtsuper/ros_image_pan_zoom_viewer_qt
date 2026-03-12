# ros_image_pan_zoom_viewer_qt

A ROS 1 image viewer with **pan and zoom** support built with **Qt5**.  
Subscribe to any `sensor_msgs/Image` topic and inspect the image interactively:
pixel-level zoom, free-drag panning, depth/mono pseudo-color visualization,
crosshair overlay, and screenshot saving.

---

## Dependencies

| Dependency | Version |
|---|---|
| ROS | Noetic (Ubuntu 20.04) |
| roscpp | – |
| image_transport | – |
| cv_bridge | – |
| sensor_msgs | – |
| OpenCV | ≥ 3.x (ships with Noetic) |
| Qt | 5.x |

Install ROS Noetic dependencies:

```bash
sudo apt install ros-noetic-cv-bridge ros-noetic-image-transport \
                 libopencv-dev qtbase5-dev
```

---

## Build

```bash
# Create / enter your catkin workspace
mkdir -p ~/catkin_ws/src
cd ~/catkin_ws/src
# Clone or copy this package here
cd ..
catkin_make          # or: catkin build
source devel/setup.bash
```

---

## Run

```bash
# Using the default topic  /camera/image_raw
rosrun ros_image_pan_zoom_viewer_qt image_viewer_qt

# Override the topic via a private parameter
rosrun ros_image_pan_zoom_viewer_qt image_viewer_qt _topic:=/your/image/topic
```

Or launch with a launch file:

```xml
<launch>
  <node pkg="ros_image_pan_zoom_viewer_qt" type="image_viewer_qt"
        name="image_viewer_qt" output="screen">
    <param name="topic" value="/camera/image_raw"/>
  </node>
</launch>
```

---

## Controls

| Key / Action | Effect |
|---|---|
| `F` | Fit image to window |
| `1` | Actual size (1 : 1) |
| `R` | Reset view (fit to window) |
| `S` | Save screenshot |
| `C` | Toggle crosshair overlay |
| `D` | Toggle depth pseudo-color (JET colormap) |
| `Esc` | Quit |
| **Mouse wheel** | Zoom (~1.2×) |
| **Shift + wheel** | Fine zoom (~1.08×) |
| **Ctrl + wheel** | Fast zoom (~1.5×) |
| **Left drag** | Pan |

### Status bar

Displays the current topic, frame counter, image size, encoding, mouse pixel
coordinates, and per-channel pixel values.

---

## Supported encodings

| Encoding | Handling |
|---|---|
| `rgb8`, `bgr8`, `rgba8` | Direct display |
| `mono8` | Grayscale display |
| `mono16`, `16UC1` | Normalized 8-bit → grayscale or pseudo-color |
| `32FC1` | Normalized 8-bit → grayscale or pseudo-color |

---

## License

MIT