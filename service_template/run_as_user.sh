#!/bin/sh

# Exit on any error and treat unset variables as errors
set -euo

OLD_UID=$1
OLD_GID=$2
shift; shift

groupadd --gid $OLD_GID --non-unique user
useradd --uid $OLD_UID --gid $OLD_GID --non-unique user

sudo -E -u user "$@"
