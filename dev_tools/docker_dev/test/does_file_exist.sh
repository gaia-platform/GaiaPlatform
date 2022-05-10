#!/usr/bin/env bash

if [[ -f $1 ]] ; then
    echo "File does exist."
    exit 0
else
    echo "File does not exist."
    exit 1
fi
