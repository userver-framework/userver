#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -o errexit -o nounset -o pipefail -o posix -x

KAFKA_VERSION=4.0.1
KAFKA_HOME=/etc/kafka

DEBIAN_FRONTEND=noninteractive sudo apt install -y openjdk-17-jdk

curl "https://dlcdn.apache.org/kafka/$KAFKA_VERSION/kafka_2.13-$KAFKA_VERSION.tgz" -o kafka.tgz
mkdir -p "$KAFKA_HOME"
tar -xzf kafka.tgz --directory="$KAFKA_HOME" --strip-components=1
rm kafka.tgz
