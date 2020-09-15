#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set(built_in_types "bool;byte;char;float32;float64;int8;uint8;int16;uint16;int32;uint32;int64;uint64;string;wstring")

#
# Converts ROS2 .msg files into DDL schemas for Gaia table creation.
#
function(ros2_msg_to_gaia_ddl)
  set(interface_list "")

  foreach(interface_arg ${ARGN})
    list(APPEND interface_list "${interface_arg}")

    set(nested_msgs "")
    get_nested_msgs(${interface_arg} nested_msgs)

    foreach(nested_msg ${nested_msgs})
      list(APPEND interface_list "${nested_msg}")
    endforeach()
  endforeach()

  list(REMOVE_DUPLICATES interface_list)
  list(SORT interface_list)

  print_list("${interface_list}")
endfunction(ros2_msg_to_gaia_ddl)

#
# Reads a ROS2 interface file to find field types that are ROS2 messages.
#
function(get_nested_msgs interface_arg nested_msgs_result)
  set(ros2_pkg "")
  string(REGEX MATCH "^[a-z0-9_]*" ros2_pkg ${interface_arg})
  set(interface_file "")
  string(REGEX MATCH "[A-Za-z0-9_\\/\\.]*$" interface_file ${interface_arg})

  find_package(${ros2_pkg})
  set(interface_file_path "${${ros2_pkg}_DIR}/../${interface_file}")

  set(file_lines "")
  # Remove comments (they begin with #) and reject leading whitespace.
  file(STRINGS ${interface_file_path} file_lines REGEX "^[^# \t\r\n]")

  set(nested_msgs "")
  foreach(file_line ${file_lines})
    set(field_type "")
    # Extract the field type, which is the first word before the space,
    # excluding the square brackets from non-scalar types.
    string(REGEX MATCH "^[A-Za-z0-9_\\/]*" field_type ${file_line})

    # Ignore built-in types because they are not nested messages.
    if("${field_type}" IN_LIST built_in_types)
      continue()
    endif()

    # If a nested message is from a different package, its field type will
    # contain a forward-slash. Otherwise, it comes from the same package
    # as its parent message.
    # Format the field type as "<package> msg/<message>.msg".
    if(field_type MATCHES "^.*\\/.*$")
      string(REPLACE "/" " msg/" field_type ${field_type})
    else()
      string(PREPEND field_type "${ros2_pkg} msg/")
    endif()
    string(APPEND field_type ".msg")

    list(APPEND nested_msgs "${field_type}")
  endforeach()

  list(REMOVE_DUPLICATES nested_msgs)

  set(deeper_msgs "")
  foreach(nested_msg ${nested_msgs})
    set(nested_pkg "")
    string(REGEX MATCH "^[a-z0-9_]*" nested_pkg ${nested_msg})
    set(nested_interface_file "")
    string(REGEX MATCH "[A-Za-z0-9_\\/\\.]*$" nested_interface_file ${nested_msg})
    set(deeper_result "")

    get_nested_msgs("${nested_pkg} ${nested_interface_file}" deeper_result)
    if(NOT "${deeper_result}" STREQUAL "")
      list(APPEND deeper_msgs "${deeper_result}")
    endif()
  endforeach()

  list(APPEND nested_msgs "${deeper_msgs}")
  list(REMOVE_DUPLICATES nested_msgs)
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
