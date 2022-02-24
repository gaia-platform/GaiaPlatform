#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide the main entry point from a -m/module use case.
"""

# PYTHON_ARGCOMPLETE_OK

import sys
from gdev.main import DockerDev


def main() -> int:
    """
    Module main entry point into the application.
    """
    DockerDev().main()
    return 0


if __name__ == "__main__":
    sys.exit(main())
