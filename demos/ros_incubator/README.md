# ROS2 incubator demo
This is a ROS2 adaptation of the original incubator demo. The "physics simulation" state of the demo is decoupled from the Gaia-specific "business logic" represented by rules and database tables.

## ROS2 build and run

1. Using **gdev**, enter a Docker container in `/build/demos/ros_incubator`.

2. Build the ROS2 source files:
```
source /opt/ros/foxy/setup.bash
colcon build
```
You only need to source `setup.bash` once. You can keep the Docker shell open, edit the source files, and run `colcon build` repeatedly as a ROS2 development workflow.

3. Launch the demo:
```
source install/local_setup.bash
ros2 run gaia_incubator incubator_manager
```
