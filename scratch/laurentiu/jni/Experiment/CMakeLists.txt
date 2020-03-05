#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.
project(Experiment)

# Add custom target for generating JNI C++ header files.
add_custom_command(
  OUTPUT Experiment.h
  COMMAND ${Java_JAVAC_EXECUTABLE} -d . -h ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/Experiment.java
  DEPENDS ${PROJECT_SOURCE_DIR}/Experiment.java)

add_custom_target(experiment_jni_headers DEPENDS Experiment.h)

# Add custom target for executing test program.
add_custom_target(Experiment ${Java_JAVA_EXECUTABLE} -Djava.library.path=. Experiment)

add_library(native_experiment SHARED Experiment.cpp)
add_dependencies(native_experiment experiment_jni_headers)
target_include_directories(native_experiment PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(native_experiment PRIVATE ${JNI_INCLUDE_DIRS})
set_target_properties(native_experiment PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(native_experiment PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
