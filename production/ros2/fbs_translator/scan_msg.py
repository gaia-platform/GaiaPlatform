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
    "-n", "--nested",
    action="store_true",
    help="Set for processing nested messages within the top-level message"
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
    if args.nested:
        field = camel_to_snake_case(msg_name, True) + "_" + field

    nested_msg_type = check_nested_msg(field_type)

    if nested_msg_type:
        # add to nested_msg_fields
        nested_msg_fields.append(
            (field, nested_msg_type[0], nested_msg_type[1])
        )
    else:
        # add to base_fields
        base_fields.append((field, field_type))

if not args.nested:
    print("MESSAGE: " + msg_name)

for field, field_type in base_fields:
    print(field_type + " " + field)

for field, pkg, field_type in nested_msg_fields:
    print(pkg + "/" + field_type + " " + field)
