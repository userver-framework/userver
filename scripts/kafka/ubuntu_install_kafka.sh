#!/bin/sh

if [ "$(uname)" = "Darwin" ]; then
  brew install openjdk@17
else
  apt install -y openjdk-17-jdk
  sudo update-alternatives --set java /usr/lib/jvm/java-17-openjdk-amd64/bin/java
fi

if [ -z ${KAFKA_PATH+x} ]; then
  if [ "$(uname)" = "Darwin" ]; then
    export KAFKA_PATH="/opt/homebrew/opt/kafka/libexec"
  else
    export KAFKA_PATH="/etc/kafka"
  fi
fi

curl https://dlcdn.apache.org/kafka/4.0.0/kafka_2.13-4.0.0.tgz -o kafka.tgz
mkdir -p ${KAFKA_PATH}
tar xf kafka.tgz --directory="${KAFKA_PATH}"
cp -r ${KAFKA_PATH}/kafka_2.13-4.0.0/* ${KAFKA_PATH}
rm -rf ${KAFKA_PATH}/kafka_2.13-4.0.0
rm kafka.tgz
