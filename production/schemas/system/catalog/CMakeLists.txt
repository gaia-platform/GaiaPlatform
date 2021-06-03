#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(catalog_schema)

process_schema_internal(DATABASE_NAME catalog)
