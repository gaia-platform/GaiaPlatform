#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide for the `cfg` subcommand entry point.
"""

from gdev.sections.run.cfg import GenRunCfg


class Cfg:
    """
    Class to provide for the `cfg` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Generate the configuration used as the basis for the dockerfile."

    @classmethod
    def cli_entrypoint(cls, options) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunCfg(options).run()
