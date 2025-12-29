#!/bin/sh

#######################################################################################################################
# Script for fast freeing disk space in Github CI                                                                     #
#######################################################################################################################

df -h

# See https://stackoverflow.com/questions/75536771/github-runner-out-of-disk-space-after-building-docker-image
sudo rm -rf /usr/share/dotnet /usr/local/lib/android /usr/lib/php* /opt/ghc \
    /usr/local/share/powershell /usr/share/swift /usr/local/.ghcup \
    /opt/hostedtoolcache/CodeQL || true
sudo docker image prune --all --force

df -h
