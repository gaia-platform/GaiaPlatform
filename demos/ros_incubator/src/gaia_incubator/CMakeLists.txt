cmake_minimum_required(VERSION 3.17) # ROS2 defaults to version 3.5
project(gaia_incubator)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

# FIXME: until gaia.cmake is installed to the cmake modules path,
# these harcoded paths are for the convenience of calling colcon without
# passing in cmake arguments.
set(GAIA_REPO "$ENV{GAIA_REPO}")
set(GAIA_BUILD_DIR "$ENV{GAIA_BUILD_DIR}")

set(CMAKE_MODULE_PATH "${GAIA_REPO}/production/cmake;${CMAKE_MODULE_PATH}")
include(gaia)

# gaia variables
set(GAIA_PROD_BUILD "${GAIA_BUILD_DIR}/production")
set(GAIA_DEMO_BUILD "${GAIA_BUILD_DIR}/demos")
set(GAIA_PROD "${GAIA_REPO}/production")
set(GAIA_INC "${GAIA_PROD}/inc")
set(FLATBUFFERS "${GAIA_REPO}/third_party/production/flatbuffers")
set(FLATBUFFERS_INC "${FLATBUFFERS}/include")
set(GENERATED_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}/generated")

# ros2 setup
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
# end of ros2 setup

set(ROS_INCUBATOR_DEMO_INCLUDES
  ${GAIA_PROD}/db/storage_engine/mock
  ${GAIA_INC}/public/common
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/public/rules
  ${GAIA_INC}/internal/rules
  ${FLATBUFFERS_INC}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp/gaia_incubator/msg
  /opt/ros/foxy/include
  ${GENERATED_OUTPUTS}
)
# end of gaia variables

# gaia setup
gaia_gen_rule_subscriptions(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gaia_logic.cpp"
    "${GENERATED_OUTPUTS}/gaia_logic_init.cpp"
    "${ROS_INCUBATOR_DEMO_INCLUDES}"
)

add_custom_target(make_generated_incubator_dir ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_OUTPUTS}
)

set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp_opt(src/barn_storage.fbs
  "--no-includes;--gen-compare;--gen-setters;--gen-events;--gaia-type-initial-value;1000"
  "${GENERATED_OUTPUTS}"
)
gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_barn_storage
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION "${GAIA_PROD_BUILD}/system/libgaia.a")
add_library(gaia_semock SHARED IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION "${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a")
add_library(gaia_rules SHARED IMPORTED GLOBAL)
set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION "${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a")
add_library(gaia_direct SHARED IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION "${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a")

# end of gaia setup

# node setup
add_library(gaia_library SHARED
  src/incubator_manager.cpp
  src/gaia_logic.cpp
  "${GENERATED_OUTPUTS}/gaia_logic_init.cpp"
)

add_dependencies(gaia_library generate_barn_storage gaia_semock)
target_include_directories(gaia_library PRIVATE ${ROS_INCUBATOR_DEMO_INCLUDES})

target_compile_options(gaia_library PUBLIC -fPIC)

target_compile_definitions(gaia_library
  PRIVATE "COMPOSITION_BUILDING_DLL"
)

ament_target_dependencies(gaia_library
  rclcpp
  rclcpp_components
)

target_link_libraries(gaia_library
  rt pthread gaia gaia_semock gaia_rules gaia_direct
)

rclcpp_components_register_node(gaia_library
  PLUGIN "incubator_manager"
  EXECUTABLE incubator_manager
)

rclcpp_components_register_node(gaia_library
  PLUGIN "gaia_logic"
  EXECUTABLE gaia_logic
)

target_include_directories(gaia_library PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

rosidl_target_interfaces(gaia_library
  ${PROJECT_NAME} "rosidl_typesupport_cpp"
)
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
