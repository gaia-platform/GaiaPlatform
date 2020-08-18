#!/usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import io
import os

def filename(filepath):
    return os.path.basename(filepath)

parser = argparse.ArgumentParser()
parser.add_argument(
    "manifest_path",
    help="File path to manifest.txt, listing JSON files generated from ROS2 messages",
    type=str
)
args = parser.parse_args()

with io.open(args.manifest_path, 'r') as manifest:
    json_files = manifest.read().splitlines()
    manifest.close()

for json_path in json_files:
    json_filename = filename(json_path)
    print(">>>> " + json_filename)

    with io.open(json_path, 'r') as json_file:
        for line in json_file:
            print(line, end="")
        print("\n")
        json_file.close()
