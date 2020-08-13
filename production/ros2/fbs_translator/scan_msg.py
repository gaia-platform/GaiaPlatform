#!/usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import importlib.util
import os
import re
import sys

def filename_no_ext(filepath):
    return os.path.splitext(os.path.basename(filepath))[0]

# Converts MyFoo to _my_foo or my_foo.
def camel_to_snake_case(camel_str, no_first_underscore=False):
    snake_str = re.sub("[A-Z]", r"_\g<0>", camel_str).lower()
    if no_first_underscore:
        snake_str = snake_str.lstrip('_')
    return snake_str

# If a field type is a nested message, return the package and message name.
def check_nested_msg(field_type):
    split_list = field_type.split('/')
    return None if (len(split_list) < 2) else tuple(split_list)

parser = argparse.ArgumentParser()
parser.add_argument(
    "-f", "--fbs_path",
    help="For nested message types, provide the filepath of the top-level FBS"
)
parser.add_argument(
    "package_name",
    help="Name of the current ROS2 package containing the message file",
    type=str
)
parser.add_argument(
    "cmake_current_binary_dir",
    help="Build directory for the current ROS2 package",
    type=str
)
parser.add_argument(
    "msg_file_path",
    help="Filepath of a ROS2 message file",
    type=str
)

args = parser.parse_args()

def get_out_file():
    if args.fbs_path:
        return open(args.fbs_path, "a")
    else:
        return open(args.cmake_current_binary_dir + "/gaia_translation.fbs", "w")

msg_name = filename_no_ext(args.msg_file_path)
msg_py_filename = camel_to_snake_case(msg_name) + ".py"
msg_py_filepath = args.cmake_current_binary_dir + "/rosidl_generator_py/" \
    + args.package_name + "/msg/" + msg_py_filename

msg_spec = importlib.util.spec_from_file_location(msg_name, msg_py_filepath)
msg_module = importlib.util.module_from_spec(msg_spec)
msg_spec.loader.exec_module(msg_module)
msg_fields = getattr(msg_module, msg_name)._fields_and_field_types

base_fields = []
nested_msg_fields = []

for field, field_type in msg_fields.items():
    nested_msg_type = check_nested_msg(field_type)

    if nested_msg_type:
        nested_msg_fields.append(
            (field, nested_msg_type[0], nested_msg_type[1])
        )
    else:
        base_fields.append((field, field_type))

out_file = get_out_file()

if not args.fbs_path:
    out_file.write("namespace ros_msgs\n\n")

out_file.write("table " + msg_name + " {\n")

for field, field_type in base_fields:
    out_file.write("    " + field + ": " + field_type + ";\n")

for field, pkg, field_type in nested_msg_fields:
    out_file.write("    " + field + ": " + pkg + "/" + field_type + ";\n")

out_file.write("}\n")

out_file.close()

for field, pkg, field_type in nested_msg_fields:
    print(pkg + " " + field_type + ".msg")
