#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.
project(HelloWorldJNI)

find_package(Java)
find_package(JNI)

if (JAVA_FOUND)
    message (STATUS "Java_JAVAC_EXECUTABLE=${Java_JAVAC_EXECUTABLE}")
    message (STATUS "Java_JAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}")

  # Add custom targets for HelloWorldJNI test.
  add_custom_target(jni_headers ${Java_JAVAC_EXECUTABLE} -d . -h ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/HelloWorldJNI.java)

  add_custom_target(HelloWorldJNI ${Java_JAVA_EXECUTABLE} -Djava.library.path=. HelloWorldJNI)
endif()

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif()

add_library(native SHARED HelloWorldJNI.cpp)
add_dependencies(native jni_headers)
target_include_directories(native PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(native PRIVATE ${JNI_INCLUDE_DIRS})
set_target_properties(native PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(native PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
