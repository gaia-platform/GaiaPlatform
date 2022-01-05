#!/bin/bash

#############################################
# Copyright (c) 2021 Gaia Platform LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
#############################################

export PGUSER=${PGUSER-postgres}

# Set up database tables.
psql -f setup.sql
