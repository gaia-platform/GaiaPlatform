#!/usr/bin/env bash

# shellcheck disable=SC2046,SC2068
PYTHONPATH=$(dirname $(realpath "$0")) python3 -m gdev $@
