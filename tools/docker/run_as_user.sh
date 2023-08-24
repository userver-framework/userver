#!/usr/bin/env bash
set -e

if ! id -u user > /dev/null 2> /dev/null; then
    DIR_UID="$(stat -c '%u' .)"
    if [ "$DIR_UID" = "0" ]; then
        useradd --create-home --no-user-group user
    else
        useradd --create-home --no-user-group --uid $DIR_UID user
    fi
fi

HOME=/home/user sudo -E -u user "$@"
