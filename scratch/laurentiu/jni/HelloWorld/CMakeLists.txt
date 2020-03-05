#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.
project(HelloWorld)

# Add custom target for building Java files and generating JNI C++ header files.
add_custom_command(
  OUTPUT HelloWorld.h
  COMMAND ${Java_JAVAC_EXECUTABLE} -d . -h ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/HelloWorld.java
  DEPENDS ${PROJECT_SOURCE_DIR}/HelloWorld.java)
add_custom_target(hello_world_jni_headers DEPENDS HelloWorld.h)

# Configure build of native C++ library.
add_library(native_hello_world SHARED HelloWorld.cpp)
add_dependencies(native_hello_world hello_world_jni_headers)
target_include_directories(native_hello_world PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(native_hello_world PRIVATE ${JNI_INCLUDE_DIRS})
set_target_properties(native_hello_world PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(native_hello_world PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

# Add custom target for executing Java program.
add_custom_target(HelloWorld ${Java_JAVA_EXECUTABLE} -Djava.library.path=. HelloWorld)
