#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

#
# Converts ROS2 .msg files into flatbuffer schemas for Gaia table creation.
#
function(gaia_ros2_msg_to_ddl MSG_FILES)
  foreach(MSG_FILE ${MSG_FILES})
    set(MSG_FILE_ABSOLUTE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${MSG_FILE}")
    execute_process(
      COMMAND "${PYTHON_EXECUTABLE}" scan_msg.py ${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR} ${MSG_FILE_ABSOLUTE_PATH}
      OUTPUT_VARIABLE MSG_TO_DDL_OUT
      WORKING_DIRECTORY "${GAIA_REPO}/production/ros2/ddl_translator"
    )
    message(NOTICE "${MSG_TO_DDL_OUT}")
  endforeach()
  message(NOTICE "========")

  execute_process(
    COMMAND bash -c "tail -n +1 ${CMAKE_CURRENT_BINARY_DIR}/json_msgs/*.json"
    OUTPUT_VARIABLE MSG_CONVERSION_DUMP
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  )

  message(NOTICE "${MSG_CONVERSION_DUMP}")
  message(NOTICE "========")

  #string(REPLACE "\n" ";" NESTED_MSG_LIST "${MSG_CONVERSION_DUMP}")
  #foreach(NESTED_MSG ${NESTED_MSG_LIST})
  #  message(NOTICE "${NESTED_MSG}")
  #endforeach()

  message(FATAL_ERROR "Stopping for testing purposes.") # remove this later
endfunction(gaia_ros2_msg_to_ddl)
