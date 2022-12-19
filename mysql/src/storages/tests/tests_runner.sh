#!/bin/bash

MYSQL_PORT="${TESTSUITE_MYSQL_PORT:-13307}"
mysql -u root -h 127.0.0.1 -P $MYSQL_PORT -e "CREATE DATABASE IF NOT EXISTS userver_mysql_test"

__BINARY__ __ARGS__
