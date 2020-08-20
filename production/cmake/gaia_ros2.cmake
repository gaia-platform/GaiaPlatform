#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

#
# Converts ROS2 .msg files into flatbuffer schemas for Gaia table creation.
#
function(gaia_ros2_msg_to_ddl SCHEMA_NAME MANIFEST_FILE MSG_FILES)
  # Overwrite the previous manifest file, if it existed.
  file(WRITE ${MANIFEST_FILE} "")

  foreach(MSG_FILE ${MSG_FILES})
    set(MSG_FILE_ABSOLUTE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${MSG_FILE}")
    execute_process(
      COMMAND "${PYTHON_EXECUTABLE}" scan_msg.py ${CMAKE_CURRENT_BINARY_DIR} ${MSG_FILE_ABSOLUTE_PATH} ${MANIFEST_FILE}
      OUTPUT_VARIABLE SCAN_MSG_OUT
      WORKING_DIRECTORY "${GAIA_REPO}/production/ros2/ddl_translator"
    )
  endforeach()

  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" generate_ddl.py ${SCHEMA_NAME} ${MANIFEST_FILE}
    OUTPUT_VARIABLE GENERATE_DDL_OUT
    WORKING_DIRECTORY "${GAIA_REPO}/production/ros2/ddl_translator"
  )
  message(NOTICE "${GENERATE_DDL_OUT}")

  message(FATAL_ERROR "Stopping for testing purposes.") # remove this later
endfunction(gaia_ros2_msg_to_ddl)
