#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

include(UseJava)

find_package(Java)
find_package(JNI)

# Java AirportDataProcessor project.
if (JAVA_FOUND AND JNI_FOUND)
  add_jar(AirportDataProcessor
    SOURCES
      graphml/AirportDataProcessor.java
      graphml/CsvParser.java)
  get_target_property(AIRPORT_DATA_PROCESSOR_JAR AirportDataProcessor JAR_FILE)

  add_custom_target(
    airport_data_processor
    ${Java_JAVA_EXECUTABLE} -cp ${AIRPORT_DATA_PROCESSOR_JAR} AirportDataProcessor)
endif()
