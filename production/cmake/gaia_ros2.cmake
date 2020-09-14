#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set(built_in_types "bool;byte;char;float32;float64;int8;uint8;int16;uint16;int32;uint32;int64;uint64;string;wstring")

#
# Converts ROS2 .msg files into DDL schemas for Gaia table creation.
#
function(ros2_msg_to_gaia_ddl ros2_pkg interface_files dependencies)
  set(interface_list "")
  find_package(${ros2_pkg})

  foreach(interface_file ${interface_files})
    list(APPEND interface_list "${ros2_pkg} ${interface_file}")

    set(nested_msgs "")
    get_nested_msgs(${ros2_pkg} ${interface_file} nested_msgs)

    foreach(nested_msg ${nested_msgs})
      list(APPEND interface_list "NESTED: ${nested_msg}")
    endforeach()
  endforeach()

  print_list("${interface_list}")
endfunction(ros2_msg_to_gaia_ddl)

#
# Reads a ROS2 interface file to find field types that are ROS2 messages.
#
function(get_nested_msgs ros2_pkg interface_file nested_msgs_result)
  find_package(${ros2_pkg})
  set(interface_file_path "${${ros2_pkg}_DIR}/../${interface_file}")
  set(nested_msgs "")
  set(file_lines "")

  file(STRINGS ${interface_file_path} file_lines REGEX "^[^#]")

  foreach(file_line ${file_lines})
    string(REGEX MATCH "^[A-Za-z0-9_\\/]*" file_line ${file_line})

    if(NOT "${file_line}" IN_LIST built_in_types)
      list(APPEND nested_msgs "${file_line}")
    endif()
  endforeach()

  set(${nested_msgs_result} "${nested_msgs}" PARENT_SCOPE)
endfunction(get_nested_msgs)

#
# Prints a cmake list to stderr.
#
function(print_list list_to_print)
  foreach(element ${list_to_print})
    message(NOTICE ${element})
  endforeach()
endfunction(print_list)

#foreach(dependency ${DEPENDENCIES})
#  find_package(${dependency})
#  message(NOTICE ">> dep: ${dependency}")
#  message(NOTICE "recursive dependencies: ${${dependency}_RECURSIVE_DEPENDENCIES}")
#  foreach(dependency_interface_file ${${dependency}_INTERFACE_FILES})
#    message(NOTICE "  -- ${${dependency}_DIR}/../${dependency_interface_file}")
#  endforeach()
#endforeach()
