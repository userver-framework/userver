#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

POSTGRESQL_VERSION=${POSTGRESQL_VERSION:=16}

apt remove -y postgresql-server-dev-${POSTGRESQL_VERSION}

# Installing postgresql-server-dev-14 without dependencies
#
# We should feel sorry because of that but we don't. The package
# pulls in ~700MB of unnecessary dependencies.
apt download postgresql-server-dev-${POSTGRESQL_VERSION}
dpkg-deb -R postgresql-server-dev-${POSTGRESQL_VERSION}* tmp_postgresql
cp -r tmp_postgresql/usr/* /usr/
rm -rf postgresql-server-dev-${POSTGRESQL_VERSION}* tmp_postgresql
