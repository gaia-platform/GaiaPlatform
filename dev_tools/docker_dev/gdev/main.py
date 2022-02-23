#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide a single entry point from the operating system.
"""

import sys
import logging
from gdev.dependency import Dependency


# pylint: disable=too-few-public-methods
class DockerDev:
    """
    Class to provide a single entry point from the operating system.
    """

    @staticmethod
    def main():
        """
        Main entry point from the operating system.
        """
        dependency = Dependency.of_sys_argv()

        logging.basicConfig(level=dependency.options.log_level)

        try:
            dependency.cli_entrypoint()
        except dependency.Exception as this_exception:
            print(f"\n{this_exception}", file=sys.stderr)
        finally:
            logging.shutdown()

        return 0


# pylint: enable=too-few-public-methods


# pylint: enable=too-few-public-methods

if __name__ == "__main__":
    DockerDev().main()
