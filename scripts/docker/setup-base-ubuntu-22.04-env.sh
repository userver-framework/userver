#!/bin/bash

# Exit on any error
set -eo pipefail

# Install build dependencies
if [ ! -f ubuntu-22.04.md ]; then
    echo "File 'ubuntu-22.04.md' not found!"
    exit 1
fi

PACKAGES_TO_INSTALL=$(cat ubuntu-22.04.md | tr '\n' ' ')
declare -a PACKAGES_FOR_MANUAL_INSTALL=(
  "postgresql-server-dev-14"
  "redis-server"
  "postgresql-14"
)
for PACKAGE in "${PACKAGES_FOR_MANUAL_INSTALL[@]}"; do   
  PACKAGES_TO_INSTALL=( "${PACKAGES_TO_INSTALL[@]/$PACKAGE}" )
done

echo "PACKAGES_TO_INSTALL=${PACKAGES_TO_INSTALL}"

apt update

DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
  ${PACKAGES_TO_INSTALL} \
  chrpath \
  curl \
  sudo \
  gnupg \
  gnupg2 \
  wget \
  dirmngr \
  python3-pip \
  locales

# Installing postgresql-server-dev-14 without dependencies
#
# We should feel sorry because of that but we don't. The package
# pulls in ~700MB of unnecessary dependencies.
apt download postgresql-server-dev-14
dpkg-deb -R postgresql-server-dev-14* tmp_postgresql
cp -r tmp_postgresql/usr/* /usr/
rm -rf postgresql-server-dev-14* tmp_postgresql

# Cleanup
apt clean all

# Set UTC timezone
TZ=Etc/UTC
ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Generate locales
locale-gen en_US.UTF-8
update-locale LC_ALL="en_US.UTF-8" LANG="en_US.UTF-8" LANGUAGE="en_US.UTF-8"
