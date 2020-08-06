#!/usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse

parser = argparse.ArgumentParser()
parser.add_argument(
    "msg_file",
    help="Filename of a ROS2 message file.",
    type=str
)

args = parser.parse_args()

print(args.msg_file)
