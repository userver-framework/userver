#!/bin/bash

# any error - exit
set -eo pipefail

# We need in git
if [ ! $(which git) ]; then
	echo "Error: Git not found"
	exit 1;
fi
# You could override those versions from command line
# Example: FMT_VERSION=8.1.1 ./prepare_image.sh
FMT_VERSION=${AMQP_VERSION:=8.1.1}
CARES_VERSION=${CARES_VERSION:=cares-1_19_1}

current_dir=$(pwd)

rm -rf $current_dir/src/

mkdir -p $current_dir/src/

cd $current_dir/src && git clone https://github.com/fmtlib/fmt.git \
	&& cd fmt && git checkout ${FMT_VERSION} && rm -rf ./.git
cd $current_dir/src && git clone https://github.com/c-ares/c-ares.git \
	&& cd c-ares && git checkout ${CARES_VERSION} && rm -rf ./.git


echo 'voluptuous >= 0.11.1
Jinja2 >= 2.10
PyYAML >= 3.13
netifaces >= 0.10.0
yandex-taxi-testsuite[] >= 0.1.19' > $current_dir/src/requirements.txt
