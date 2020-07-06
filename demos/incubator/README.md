# Incubator Demo
A demo of a rule-based control system for controlling the temperature of several incubators.

This currently contains a ROS2 workspace. ROS2 integration is in progress.

## Build Instructions
1. Build Gaia Platform production code.
2. Run cmake and supplement the production build directory with the variable `GAIA_PROD_BUILD`.
```
> mkdir build && cd build
> cmake .. -DCMAKE_BUILD_TYPE=Debug -DGAIA_PROD_BUILD=../../../production/build
> make
```

## ROS2 Build and Run
Be aware that **gdev** takes its cache from the latest commit, not the current state of the directory.

1. Run **gdev** in this directory and wait:
`gdev gen shell`
It will put you in a bash shell in a Docker container.

2. Build the ROS2 source files:
```
source /opt/ros/foxy/setup.bash
colcon build
```
You only need to source `setup.bash` once. You can keep the Docker shell open, edit the source files, and run `colcon build` repeatedly as a ROS2 development workflow.

3. Run the incubator manager node:
```
source install/local_setup.bash
ros2 run gaia_incubator incubator_manager
```

4. **(Optional)** Open another Docker terminal to monitor ROS2 topics:

Open another terminal in `demos/incubator`.

`gdev gen shell`

Wait for a Docker shell to open.
```
source /opt/ros/foxy/setup.bash
ros2 topic echo temp
```
