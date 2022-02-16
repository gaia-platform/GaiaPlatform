#!/usr/bin/env bash

# gdev won't run unless ssh-agent is running.
# If ssh-agent is running, then SSH_AUTH_SOCK will be set and its value will be a socket file.
if [[ ! -S "${SSH_AUTH_SOCK}" ]]; then
    cat <<-'END'
ssh-agent is required for gdev to clone git repositories.
Please start ssh-agent and add your git private keys:
-
    eval `ssh-agent`
    ssh-add
-
END
    exit 1
fi

# shellcheck disable=SC2068
pipenv run pytest -ra $@
