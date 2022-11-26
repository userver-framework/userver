#!/usr/bin/env sh

cmake --build build_release/ --target userver-core_benchmark
build_release/userver/core/userver-core_benchmark --benchmark_filter='(Sl)|(L)[rf]u'
