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

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

find_package(rclcpp REQUIRED)
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
)

ament_export_dependencies(rosidl_default_runtime)

# incubator_manager node
add_executable(incubator_manager src/incubator_manager.cpp)
ament_target_dependencies(incubator_manager rclcpp)

target_include_directories(incubator_manager PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

rosidl_target_interfaces(incubator_manager
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

# gaia_logic node
add_executable(gaia_logic src/gaia_logic.cpp)
ament_target_dependencies(gaia_logic rclcpp)

target_include_directories(gaia_logic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

rosidl_target_interfaces(gaia_logic
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

# end of nodes

install(TARGETS
  incubator_manager
  gaia_logic
  EXPORT export_${PROJECT_NAME}
  DESTINATION lib/${PROJECT_NAME})


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

ament_package()
