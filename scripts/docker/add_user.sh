#!/bin/bash

# Exit on any error
set -eo pipefail

DIR_UID="$(stat -c '%u' .)"
if [ "$DIR_UID" = "0" ]; then
    useradd --create-home --shell /bin/bash --no-user-group user
else
    useradd --create-home --shell /bin/bash --no-user-group --uid $DIR_UID user
fi
