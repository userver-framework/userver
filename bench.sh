#!/usr/bin/env sh

cmake --build build_release/ --target userver-core_benchmark
cd "$(dirname "$0")" \
    && build_release/userver/core/userver-core_benchmark \
    --benchmark_filter='(Sl)|(L)[rf]u' --benchmark_format=json \
    | ./plots.py
