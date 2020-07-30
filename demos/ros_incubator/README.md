# ROS2 incubator demo
This is a ROS2 adaptation of the original incubator demo. The "physics simulation" state of the demo is decoupled from the Gaia-specific "business logic" represented by rules and database tables.

## ROS2 build and run

1. Using **gdev**, enter a Docker container in this demo's directory.
```
user@host:~/GaiaPlatform/demos/ros_incubator$ gdev run
```
2. Build the ROS2 source files:
```
user@container:/build/demos/ros_incubator$ source /opt/ros/foxy/setup.bash
user@container:/build/demos/ros_incubator$ colcon build
```
You only need to source `setup.bash` once. You can keep the Docker shell open, edit the source files, and run `colcon build` repeatedly as a ROS2 development workflow.

3. Launch the demo:
```
user@container:/build/demos/ros_incubator$ source install/local_setup.bash
user@container:/build/demos/ros_incubator$ ros2 launch gaia_incubator incubator.launch.py
```
If you aren't adding, renaming, or deleting launch files, then you only need to run `local_setup.bash` once.
