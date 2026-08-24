#!/bin/bash

# Install MariaDB 10.11 client/dev packages and server from the official
# MariaDB apt repository.
#
# Resolves https://deb.mariadb.org to a concrete mirror and pins it in apt.
# Workaround for https://jira.mariadb.org/browse/MDBF-651: mirrorbits may
# repeatedly redirect apt to an unreachable mirror; Acquire::Retries does not
# help because each retry is sent to the same bad mirror again.

set -euo pipefail

readonly MARIADB_CODENAME="$(lsb_release -cs)"
readonly MARIADB_KEYRING=/usr/share/keyrings/mariadb.gpg
readonly MARIADB_LIST=/etc/apt/sources.list.d/mariadb.list

curl \
    --fail \
    --silent \
    --show-error \
    --location \
    --retry 5 \
    --retry-all-errors \
    https://mariadb.org/mariadb_release_signing_key.pgp \
    | gpg --dearmor -o "${MARIADB_KEYRING}"
chmod a+r "${MARIADB_KEYRING}"

MARIADB_MIRROR="$(
    curl \
        --fail \
        --silent \
        --show-error \
        --location \
        --retry 5 \
        --retry-all-errors \
        --output /dev/null \
        --write-out '%{url_effective}' \
        "https://deb.mariadb.org/10.11/ubuntu/dists/${MARIADB_CODENAME}/InRelease" \
        | sed 's|/dists/.*||'
)"
readonly MARIADB_MIRROR

echo "deb [arch=amd64,arm64,ppc64el signed-by=${MARIADB_KEYRING}] ${MARIADB_MIRROR} ${MARIADB_CODENAME} main" \
    >"${MARIADB_LIST}"

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends libmariadb-dev mariadb-server
