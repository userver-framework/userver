#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

# Install a proper compilation toolchain
apt update
DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    clang-18 \
    lld-18 \
    llvm-18 \
    clangd-18 \
    clang-format-18 \
    clang-tidy-18 \
    lldb-18 \
    libclang-rt-18-dev
apt clean all

# Prefer clang-18 toolchain by default
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100
update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100
update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-18 100
update-alternatives --install /usr/bin/ld ld /usr/bin/lld 100
update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-18 100
update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-18 100
update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 100
update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100

# Create user "user" for non-root access to services in devcontainer
USERNAME=user
USER_UID=1000
USER_GID=$USER_UID
groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME
