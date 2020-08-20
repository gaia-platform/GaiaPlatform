cmake_minimum_required(VERSION 3.17)
project(gaia_pkg)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

if(NOT DEFINED "${GAIA_REPO}")
  set(GAIA_REPO "$ENV{GAIA_REPO}")
endif()
if(NOT DEFINED "${GAIA_BUILD_DIR}")
  set(GAIA_BUILD_DIR "$ENV{GAIA_BUILD_DIR}")
endif()
set(CMAKE_MODULE_PATH "${GAIA_REPO}/production/cmake;${CMAKE_MODULE_PATH}")

include(gaia_ros2)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(gaia_pkg_interfaces REQUIRED)

set(MSG_FILES
  "msg/Test.msg"
  "msg/TestTwo.msg"
)

rosidl_generate_interfaces(${PROJECT_NAME}
  ${MSG_FILES}
  DEPENDENCIES builtin_interfaces gaia_pkg_interfaces
)

gaia_ros2_msg_to_ddl(
  "ros_msgs"
  "${PROJECT_BINARY_DIR}/manifest.txt"
  "${MSG_FILES}"
)

ament_export_dependencies(rosidl_default_runtime)

add_library(gaia_pkg_shared_lib SHARED
  src/gaia_node.cpp
)
target_compile_options(gaia_pkg_shared_lib PUBLIC -fPIC)
target_compile_definitions(gaia_pkg_shared_lib
  PRIVATE "COMPOSITION_BUILDING_DLL"
)

ament_target_dependencies(gaia_pkg_shared_lib
  rclcpp
  rclcpp_components
)

rclcpp_components_register_node(gaia_pkg_shared_lib
  PLUGIN "gaia_node"
  EXECUTABLE gaia_node
)

target_include_directories(gaia_pkg_shared_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

rosidl_target_interfaces(gaia_pkg_shared_lib
  ${PROJECT_NAME} "rosidl_typesupport_cpp"
)

install(TARGETS
  gaia_pkg_shared_lib
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()
