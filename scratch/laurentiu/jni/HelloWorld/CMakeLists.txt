#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.
project(HelloWorld)

# Build java code and package it in a jar file. Also generate native headers.
add_jar(HelloWorld HelloWorld.java GENERATE_NATIVE_HEADERS hello_world_jni_headers DESTINATION ${PROJECT_BINARY_DIR})
get_target_property(HELLO_WORLD_JAR HelloWorld JAR_FILE)

# Configure build of native C++ library.
add_library(native_hello_world SHARED HelloWorld.cpp)
target_include_directories(native_hello_world PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(native_hello_world PRIVATE ${JNI_INCLUDE_DIRS})
target_link_libraries(native_hello_world PRIVATE hello_world_jni_headers)
set_target_properties(native_hello_world PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(native_hello_world PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

# Add a test that executes the Java program.
add_test(NAME test_jni_hello_world COMMAND ${Java_JAVA_EXECUTABLE} -Djava.library.path=. -cp ${HELLO_WORLD_JAR} HelloWorld)
