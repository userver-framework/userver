#!/bin/sh

sudo apt install -y openjdk-17-jdk
java -version
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export PATH=$PATH:$JAVA_HOME/bin


curl https://dlcdn.apache.org/kafka/4.0.0/kafka_2.13-4.0.0.tgz -o kafka.tgz
mkdir -p /etc/kafka
tar xf kafka.tgz --directory=/etc/kafka
cp -r /etc/kafka/kafka_2.13-4.0.0/* /etc/kafka/
rm -rf /etc/kafka/kafka_2.13-4.0.0
rm kafka.tgz
