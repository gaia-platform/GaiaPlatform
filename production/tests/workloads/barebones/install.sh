#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

INSTALL_DIRECTORY=$1

if ! cp -rf "$SCRIPTPATH" "$INSTALL_DIRECTORY"; then
    echo "Project cannot be copied into the '$(realpath "$INSTALL_DIRECTORY")' directory."
    exit 1
fi

# ...then go into that directory.
if ! cd "$INSTALL_DIRECTORY"; then
    echo "Cannot change the current directory to the '$(realpath "$INSTALL_DIRECTORY")' directory."
    exit 1
fi

echo "Project has been copied into the specified directory."
