#!/bin/sh

sudo apt install -y default-jre

curl https://dlcdn.apache.org/kafka/3.8.0/kafka_2.13-3.8.0.tgz -o kafka.tgz
mkdir -p /etc/kafka
tar xf kafka.tgz --directory=/etc/kafka
cp -r /etc/kafka/kafka_2.13-3.8.0/* /etc/kafka/
rm -rf /etc/kafka/kafka_2.13-3.8.0
rm kafka.tgz
