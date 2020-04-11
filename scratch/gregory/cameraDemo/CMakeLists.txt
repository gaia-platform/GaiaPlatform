cmake_minimum_required(VERSION 3.11)



#list(APPEND CMAKE_MODULE_PATH 
#  ${LLVM_LIB_PATH}/lib/cmake/clang
#  ${LLVM_LIB_PATH}/lib/cmake/llvm
# )

set(camera_demo_SRCS
  
  ../ruleInit.cpp
  ../rules.cpp
  ../demo.cpp
  ../cv.cpp
)

set(CMAKE_CXX_FLAGS
      "-std=c++0x -Wall -Wextra -ggdb -Wno-unused-parameter -ggdb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ggdb -pthread")

add_executable(camera_demo ${camera_demo_SRCS})

target_link_directories(camera_demo
  PRIVATE
  /usr/local/lib
  ${CMAKE_BINARY_DIR}/../../../../production/build/direct_access
  ${CMAKE_BINARY_DIR}/../../../../production/build/rules/event_manager
  ${CMAKE_BINARY_DIR}/../../../../production/build/db/storage_engine
  ${CMAKE_BINARY_DIR}/../../../../production/build/system
)

include_directories(
  ..
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/rules
  ${CMAKE_BINARY_DIR}/../../../../production/inc/internal/common 
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/common 
  ${CMAKE_BINARY_DIR}/../../../../third_party/production/flatbuffers/include 
  ${CMAKE_BINARY_DIR}/../../../../production/db/storage_engine/mock
  ${CMAKE_BINARY_DIR}/../../../../production/build/rules/event_manager/generated 
  /usr/local/include/opencv4
)

target_link_libraries(camera_demo
  PRIVATE
  gaia_rules
  gaia_semock
  gaia
  gaia_direct
  opencv_gapi
  opencv_stitching
  opencv_aruco
  opencv_bgsegm
  opencv_bioinspired
  opencv_ccalib
  opencv_dnn_objdetect
  opencv_dnn_superres
  opencv_dpm
  opencv_face
  opencv_freetype
  opencv_fuzzy
  opencv_hfs
  opencv_img_hash
  opencv_intensity_transform
  opencv_line_descriptor
  opencv_quality
  opencv_rapid
  opencv_reg
  opencv_rgbd
  opencv_saliency
  opencv_stereo
  opencv_structured_light
  opencv_superres
  opencv_surface_matching
  opencv_tracking
  opencv_videostab
  opencv_xfeatures2d
  opencv_xobjdetect
  opencv_xphoto
  opencv_shape
  opencv_highgui
  opencv_datasets
  opencv_plot
  opencv_text
  opencv_ml
  opencv_dnn
  opencv_phase_unwrapping
  opencv_optflow
  opencv_ximgproc
  opencv_videoio
  opencv_video
  opencv_imgcodecs
  opencv_objdetect
  opencv_calib3d
  opencv_features2d
  opencv_flann
  opencv_photo
  opencv_imgproc
  opencv_core
  m
  
  rt
)

