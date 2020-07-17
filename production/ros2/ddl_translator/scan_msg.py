#!/usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import importlib.util
import io
import json
import os
from pathlib import Path
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
json_directory = os.path.join(args.cmake_current_binary_dir, "json_msgs")

class Message:
    def __init__(self, name, package):
        self.name = name
        self.package = package
        self.fields = {}
        self.nested_fields = {}

    def __eq__(self, other):
        if isinstance(other, Message):
            return (self.name == other.name and self.package == other.package)
        else:
            return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash((self.name, self.package))

    def add_field(self, field, type, pkg=None):
        if pkg:
            self.nested_fields[field] = (type, pkg)
        else:
            self.fields[field] = type

    def write_json(self, directory):
        filename = self.package + "__" + self.name + ".json"
        Path(directory).mkdir(parents=True, exist_ok=True)

        with io.open(os.path.join(directory, filename), 'w') as json_file:
            json_data = {
                "name": self.name, "package": self.package,
                "fields": self.fields, "nested_fields": self.nested_fields
            }
            json.dump(json_data, json_file, indent=4, sort_keys=True)
            json_file.close()
            print(filename)

def get_base_msg_module(msg_name, pkg, build_dir, msg_path):
    msg_py_filename = camel_to_snake_case(msg_name, False) + ".py"
    msg_py_path = build_dir + "/rosidl_generator_py/" + pkg + "/msg/" \
        + msg_py_filename

    msg_spec = importlib.util.spec_from_file_location(msg_name, msg_py_path)
    msg_module = importlib.util.module_from_spec(msg_spec)
    msg_spec.loader.exec_module(msg_module)

    return msg_module

def scan_msg_module(msg_name, msg_pkg, msg_module):
    msg = Message(name=msg_name, package=msg_pkg)
    msg_fields = getattr(msg_module, msg_name)._fields_and_field_types

    for field, field_type in msg_fields.items():
        nested_msg_type = check_nested_msg(field_type)

        if nested_msg_type:
            msg.add_field(
                field=field, type=nested_msg_type[1], pkg=nested_msg_type[0]
            )
        else:
            msg.add_field(field=field, type=field_type)

    msg.write_json(json_directory)

    for field in msg.nested_fields:
        nested_msg = msg.nested_fields[field]
        type = nested_msg[0]
        pkg = nested_msg[1]
        submodule_name = "." + camel_to_snake_case(type, False)

        nested_module = importlib.import_module(
            submodule_name, pkg + ".msg"
        )

        scan_msg_module(msg_name=type, msg_pkg=pkg, msg_module=nested_module)

base_msg_name = filename_no_ext(args.msg_file_path)
base_msg_module = get_base_msg_module(
    msg_name=base_msg_name,
    pkg=args.package_name,
    build_dir=args.cmake_current_binary_dir,
    msg_path=args.msg_file_path
)

scan_msg_module(base_msg_name, args.package_name, base_msg_module)
