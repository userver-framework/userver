#!/bin/sh

find_docker() {
    DOCKER_BINPATH=$(which docker)
    if [ "x$DOCKER_BINPATH" = "x" ]; then
        return 1
    fi
    return 0
}

check_run_docker() {
    $DOCKER_BINPATH info
    error_code=$?
    if [ $error_code -ne 0 ]; then
        return 1
    fi
    return 0
}

find_docker || die "Docker is not found."
check_run_docker || die "Docker is not running"
