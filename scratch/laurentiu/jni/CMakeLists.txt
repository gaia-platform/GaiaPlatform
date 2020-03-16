#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

include(UseJava)

find_package(Java)
find_package(JNI)

if (JAVA_FOUND)
    message (STATUS "Java_JAVAC_EXECUTABLE=${Java_JAVAC_EXECUTABLE}")
    message (STATUS "Java_JAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}")
endif()

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif()

# Add individual folders.
if (JAVA_FOUND AND JNI_FOUND)
   add_subdirectory(HelloWorld)
   add_subdirectory(Experiment)
endif()

