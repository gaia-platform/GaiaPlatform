#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.
project(HelloWorldJNI)

find_program(JAVAC "javac")
find_program(JAVA "java")

# Add custom targets for HelloWorldJNI test.
if (JAVAC)
  add_custom_target(JNI ${JAVAC} -d . -h ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/HelloWorldJNI.java)
endif()
if (JAVA)
  add_custom_target(HelloWorldJNI ${JAVA} -Djava.library.path=. HelloWorldJNI)
endif()

add_library(native SHARED HelloWorldJNI.cpp)
add_dependencies(native JNI)
target_include_directories(native PUBLIC ${PROJECT_BINARY_DIR})
set_target_properties(native PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -I/usr/lib/jvm/java-11-openjdk-amd64/include -I/usr/lib/jvm/java-11-openjdk-amd64/include/linux")
set_target_properties(native PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
