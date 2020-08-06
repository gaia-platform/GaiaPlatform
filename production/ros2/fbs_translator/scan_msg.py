#!/usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import re
import os

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

print(args.package_name)
print(args.cmake_current_binary_dir)
print(args.msg_file_path)

msg_py_filename = re.sub("[A-Z]", r"_\g<0>",
    os.path.splitext(os.path.basename(args.msg_file_path))[0]
).lower() + ".py"

print(msg_py_filename)
