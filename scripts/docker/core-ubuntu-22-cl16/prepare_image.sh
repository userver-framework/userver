#!/bin/bash

# any error - exit
set -eo pipefail

# We need in git
if [ ! $(which git) ]; then
	echo "Error: Git not found"
	exit 1;
fi

current_dir=$(pwd)

rm -rf $current_dir/src/

mkdir -p $current_dir/src/

echo 'voluptuous >= 0.11.1
Jinja2 >= 2.10
PyYAML >= 3.13
yandex-taxi-testsuite[] >= 0.1.19' > $current_dir/src/requirements.txt
