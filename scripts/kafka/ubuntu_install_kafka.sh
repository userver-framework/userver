#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -o errexit -o nounset -o pipefail -o posix -x

KAFKA_VERSION=4.3.1
KAFKA_HOME=/etc/kafka
KAFKA_URL="https://www.apache.org/dyn/closer.lua/kafka/${KAFKA_VERSION}/kafka_2.13-${KAFKA_VERSION}.tgz?action=download"

DEBIAN_FRONTEND=noninteractive sudo apt install -y openjdk-17-jdk

mkdir -p "$KAFKA_HOME"
curl -fsSL "$KAFKA_URL" | tar -xzf - --directory="$KAFKA_HOME" --strip-components=1
