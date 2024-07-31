#!/bin/sh
PYTHONPATH=../samples/hello_service/ gdb --args /home/vitja/work/userver-oss/userver/build/samples/hello_service/userver-samples-hello_service --config=../samples/hello_service/static_config.yaml
