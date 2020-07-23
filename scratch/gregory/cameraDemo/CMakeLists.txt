cmake_minimum_required(VERSION 3.11)

project(camera_demo VERSION 0.1.0)

include(${CMAKE_BINARY_DIR}/../../../../production/cmake/gaia.cmake)

#list(APPEND CMAKE_MODULE_PATH 
#  ${LLVM_LIB_PATH}/lib/cmake/clang
#  ${LLVM_LIB_PATH}/lib/cmake/llvm
# )

#find_package(OpenCV 4.3.0 REQUIRED)

set(CMAKE_CXX_FLAGS
      "-std=c++0x -Wall -Wextra -ggdb -Wno-unused-parameter -ggdb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ggdb -pthread")

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/cameraDemo/generated")
add_custom_target(make_generated_cameraDemo_dir ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_OUTPUTS}
  )
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
set(GAIA_PROD_BUILD ${CMAKE_BINARY_DIR}/../../../../production/build)
gaia_compile_flatbuffers_schema_to_cpp_opt(cameraDemo.fbs "--no-includes;--gen-compare;--gaia-type-initial-value;1000" "${GENERATED_OUTPUTS}")
gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  add_custom_target(generate_camera_demo
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

set (CAMERA_DEMO_INCLUDES
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/rules
  ${CMAKE_BINARY_DIR}/../../../../production/inc/internal/rules
  ${CMAKE_BINARY_DIR}/../../../../production/inc/internal/common
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/common
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/direct_access
  ${CMAKE_BINARY_DIR}/../../../../third_party/production/flatbuffers/include
  ${CMAKE_BINARY_DIR}/../../../../production/db/storage_engine/mock
  ${CMAKE_BINARY_DIR}/../../../../production/build/rules/event_manager/generated
  ${GENERATED_OUTPUTS}
  ${OpenCV_INCLUDE_DIR}
  )

include_directories(${CAMERA_DEMO_INCLUDES})

# GAIA_DEMO_BUILD used by gaia_gen_rule_subscriptions.
set(GAIA_DEMO_BUILD ${CMAKE_BINARY_DIR}/../../../../demos/build)
gaia_gen_rule_subscriptions(
  "../rules.cpp"
  "${GENERATED_OUTPUTS}/rules_init.cpp"
  "${CAMERA_DEMO_INCLUDES}"
)

set(CAMERA_DEMO_SRCS
  "${GENERATED_OUTPUTS}/rules_init.cpp"
  ../rules.cpp
  ../demo.cpp
  ../cv.cpp
  )

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/system/libgaia.a)

add_library(gaia_semock SHARED IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a)

add_library(gaia_rules SHARED IMPORTED GLOBAL)
set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a)

add_library(gaia_direct SHARED IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

add_executable(camera_demo ${CAMERA_DEMO_SRCS})
add_dependencies(camera_demo generate_camera_demo)

target_link_libraries(camera_demo
  PRIVATE
  gaia_rules
  gaia_semock
  gaia
  gaia_direct
  ${OpenCV_LIBS}
  m
  rt
)

