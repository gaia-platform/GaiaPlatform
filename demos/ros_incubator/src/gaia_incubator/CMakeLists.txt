cmake_minimum_required(VERSION 3.5)
project(gaia_incubator)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)

set(msg_files
  "msg/Temp.msg"
  "msg/FanState.msg"
  "msg/AddIncubator.msg"
  "msg/AddSensor.msg"
  "msg/AddFan.msg"
)

rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
  DEPENDENCIES builtin_interfaces
)

ament_export_dependencies(rosidl_default_runtime)

# node setup
add_library(gaia_library SHARED
  src/incubator_manager.cpp
  src/gaia_logic.cpp)

target_compile_definitions(gaia_library
  PRIVATE "COMPOSITION_BUILDING_DLL")
ament_target_dependencies(gaia_library
  "rclcpp"
  "rclcpp_components")

rclcpp_components_register_node(gaia_library
  PLUGIN "incubator_manager"
  EXECUTABLE incubator_manager)

rclcpp_components_register_node(gaia_library
  PLUGIN "gaia_logic"
  EXECUTABLE gaia_logic)

target_include_directories(gaia_library PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
rosidl_target_interfaces(gaia_library
  ${PROJECT_NAME} "rosidl_typesupport_cpp")
# end node setup

install(TARGETS
  gaia_library
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

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
