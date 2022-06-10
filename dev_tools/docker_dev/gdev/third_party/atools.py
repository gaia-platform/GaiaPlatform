#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to install the atools package if it is not already installed.
"""

from pathlib import Path as _Path

# pylint: disable=import-self, unused-import
try:
    from atools import memoize
except ImportError:
    from subprocess import check_call as _check_call

    _check_call("python3 -m pip install atools".split())

    from atools import memoize
# pylint: enable=import-self, unused-import

memoize_db = memoize(db_path=_Path.home() / ".memoize.gdev")
