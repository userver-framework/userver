#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

DEBIAN_FRONTEND=noninteractive sudo apt install -y openjdk-17-jdk

curl https://dlcdn.apache.org/kafka/4.0.1/kafka_2.13-4.0.1.tgz -o kafka.tgz
mkdir -p /etc/kafka
tar xf kafka.tgz --directory=/etc/kafka
cp -r /etc/kafka/kafka_2.13-4.0.1/* /etc/kafka/
rm -rf /etc/kafka/kafka_2.13-4.0.1
rm kafka.tgz
