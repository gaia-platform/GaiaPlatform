#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

export PGUSER=${PGUSER-postgres}

# Set up database tables.
psql -f setup.sql
