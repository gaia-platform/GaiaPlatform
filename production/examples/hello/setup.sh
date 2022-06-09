#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

export PGUSER=${PGUSER-postgres}

# Set up database tables.
psql -f setup.sql
