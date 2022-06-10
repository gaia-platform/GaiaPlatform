#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to install the autocomplete package if it is not already installed.
"""

# pylint: disable=import-self, unused-import
try:
    from argcomplete import autocomplete
    from argcomplete.completers import FilesCompleter
except ImportError:
    from subprocess import check_call as _check_call

    _check_call("python3 -m pip install argcomplete".split())

    from argcomplete import autocomplete  # noqa: F401
    from argcomplete.completers import FilesCompleter  # noqa: F401
# pylint: enable=import-self, unused-import
