#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

apt install curl lsb-release

# Download mongo's GPG key
curl -fsSL https://www.mongodb.org/static/pgp/server-8.0.asc | gpg --dearmor -o /usr/share/keyrings/mongodb.gpg
chmod a+r /usr/share/keyrings/mongodb.gpg

# Add mongo to apt
echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb.gpg ] https://repo.mongodb.org/apt/ubuntu $(lsb_release -cs)/mongodb-org/8.0 multiverse" \
    | tee /etc/apt/sources.list.d/mongodb-org-8.0.list

apt update
apt install -y --no-install-recommends mongodb-org
