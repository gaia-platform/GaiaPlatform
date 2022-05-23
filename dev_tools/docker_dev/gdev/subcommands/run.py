#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide for the `run` subcommand entry point.
"""

from gdev.sections.run.run import GenRunRun
from gdev.sections._custom.run import GenCustomRun


class Run:
    """
    Class to provide for the `run` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Build the image, if required, and execute a container for GaiaPlatform development."

    @classmethod
    def cli_entrypoint(cls, options) -> None:
        """
        Execution entrypoint for this module.
        """
        run = GenRunRun(options)
        if options.mixins:
            run = GenCustomRun(options=options, base_run=run)

        run.run()
