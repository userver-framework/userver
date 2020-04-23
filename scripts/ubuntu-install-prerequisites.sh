#!/bin/bash -e

CURRENT_DISTRIB=bionic
SCRIPTS_PATH=$(dirname "$(realpath "$0")")
YANDEX_ARCHIVE_KEY_ID=7FCD11186050CD1A

ESSENTIAL_PACKAGES="git make cmake clang++-7 libstdc++-7-dev lld-7 clang-format-7 taxi-deps-py3-2"
USERVER_BUILDDEPS=" \
  libssl-dev libyandex-taxi-jemalloc-dev zlib1g-dev \
  libboost-filesystem-dev libboost-locale-dev libboost-program-options-dev libboost-regex-dev libboost-thread-dev \
  libcctz-dev libcrypto++-dev libev-dev libfmt-dev libhttp-parser-dev libyaml-cpp-dev \
  libgcrypt11-dev libkrb5-dev libnghttp2-dev libyandex-taxi-curl4-openssl-dev \
  libhiredis-dev \
  libyandex-taxi-mongo-c-driver-dev \
  libpq-dev postgresql-server-dev-10 libldap2-dev \
  libyandex-taxi-grpc++-dev libyandex-taxi-c-ares-dev yandex-taxi-protobuf-compiler-grpc \
"
OPTIONAL_PACKAGES="ccache redis-server mongodb postgresql-10"

# issue sudo token to not remind of it in every message
if ! sudo -n /bin/true; then
  echo "This script runs most operations as a superuser, please provide password."
  sudo /bin/true
fi

# ensure we have Yandex archive key installed
if ! apt-key adv --list-keys "${YANDEX_ARCHIVE_KEY_ID}" >/dev/null 2>&1; then
  echo "Installing Yandex APT GPG key"
  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys "${YANDEX_ARCHIVE_KEY_ID}"
fi

# now it's safe to install our sources.list
"${SCRIPTS_PATH}/make-sources-list.sh"

# sanity check
source /etc/lsb-release
case "${DISTRIB_CODENAME}" in
  xenial)
    BAD_DISTRIB=bionic
    ;;
  bionic)
    BAD_DISTRIB=xenial
    ;;
  *)
    echo "You are using unsupported OS release (${DISTRIB_CODENAME}), here be dragons. Consider switching to ${CURRENT_DISTRIB}."
esac
if [ -n "${BAD_DISTRIB}" ] && grep -rn --color=always "${BAD_DISTRIB}" /etc/apt/sources.list*; then
  echo "You have repos for ${BAD_DISTRIB} in these locations while running ${DISTRIB_CODENAME}, remove them first." >&2
  exit 1
fi

echo "Installing essential packages"

# update package cache
sudo apt update

# install packages required for cmake/codegen
sudo apt install -y ${ESSENTIAL_PACKAGES}

# TODO: use codegen to make package list

echo "Installing common build dependencies"

# install userver build dependencies
sudo apt install -y ${USERVER_BUILDDEPS}

echo
echo "For better experience you might want to install packages: ${OPTIONAL_PACKAGES}, but they are optional."
