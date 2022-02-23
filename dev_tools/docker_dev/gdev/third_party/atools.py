#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from pathlib import Path as _Path

try:
    from atools import memoize
except ImportError:
    from subprocess import check_call as _check_call

    _check_call('python3 -m pip install atools'.split())

    from atools import memoize


memoize_db = memoize(db_path=_Path.home() / '.memoize.gdev')
