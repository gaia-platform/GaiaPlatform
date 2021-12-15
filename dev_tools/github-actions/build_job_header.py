#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to create a job header for a GitHub Actions file.
"""

import sys
import argparse

__JOB_HEADER = """---
name: Main

on:
  push:
    branches:
      - master
  pull_request:
    branches:
      - master
      - jack/gha

jobs:"""


def __process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Perform actions based on a graph of GDEV configuration files."
    )
    return parser.parse_args()


def process_script_action():
    """
    Process the creation of the header for the new GitHub Actions yaml file.
    """
    _ = __process_command_line()

    print(__JOB_HEADER)


sys.exit(process_script_action())
