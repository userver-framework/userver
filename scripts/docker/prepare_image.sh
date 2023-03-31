#!/bin/bash

# any error - exit
set -eo pipefail

# We need in git
if [ ! $(which git) ]; then
	echo "Error: Git not found"
	exit 1;
fi

# You could override those versions from command line
# Example: AMQP_VERSION=v4.3.17 ./prepare_image.sh
AMQP_VERSION=${AMQP_VERSION:=v4.3.18}
CLICKHOUSE_VERSION=${CLICKHOUSE_VERSION:=v2.3.0}
API_COMMON_PROTOS_VERSION=${API_COMMON_PROTOS_VERSION:=v.1.50.0}
GRPC_VERSION=${GRPC_VERSION:=v1.52.0}


current_dir=$(pwd)

rm -rf $current_dir/src/

mkdir -p $current_dir/src/


cd $current_dir/src && git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git \
	&& mv AMQP-CPP/ amqp-cpp/ && cd amqp-cpp && git checkout ${AMQP_VERSION} && rm -rf ./.git
cd $current_dir/src && git clone https://github.com/ClickHouse/clickhouse-cpp.git \
	&& cd clickhouse-cpp && git checkout ${CLICKHOUSE_VERSION} && rm -rf ./.git
cd $current_dir/src && git clone https://github.com/googleapis/api-common-protos.git \
	&& cd api-common-protos && git checkout ${API_COMMON_PROTOS_VERSION} && rm -rf ./.git
cd $current_dir/src && git clone https://github.com/grpc/grpc.git \
	&& cd grpc && git checkout ${GRPC_VERSION} && git submodule update --init && rm -rf ./.git

echo 'voluptuous >= 0.11.1
Jinja2 >= 2.10
PyYAML >= 3.13
yandex-taxi-testsuite[mongodb,postgresql-binary,redis,clickhouse,rabbitmq] >= 0.1.6.1
grpcio >= 1.50.0
grpcio-tools >= 1.50.0
psycopg2 >= 2.7.5
yandex-pgmigrate' > $current_dir/src/requirements.txt
