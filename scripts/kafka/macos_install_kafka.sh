#!/bin/sh

brew install openjdk@17

export KAFKA_PATH="/opt/homebrew/opt/kafka/libexec"
curl https://dlcdn.apache.org/kafka/4.0.0/kafka_2.13-4.0.0.tgz -o kafka.tgz
mkdir -p ${KAFKA_PATH}
tar xf kafka.tgz --directory="${KAFKA_PATH}"
cp -r ${KAFKA_PATH}/kafka_2.13-4.0.0/* ${KAFKA_PATH}
rm -rf ${KAFKA_PATH}/kafka_2.13-4.0.0
rm kafka.tgz