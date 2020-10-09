#!/bin/bash

err_failure() {
  echo "Prerequisites installation failed!"
  [ -z "${USERVER_FORCE}" ] && exit 1
}
trap err_failure ERR

warn_held_packages() {
  local held
  held=$({ printf '%s\n' "$@"; apt-mark showhold; } | sort | uniq -d)
  for pkg in $held; do
    echo "WARN: The following package is on hold and will not be upgraded: ${pkg}"
  done
}

skip_held_packages() {
  { printf '%s\n' "$@"; apt-mark showhold | sed p; } | sort | uniq -u
}

CURRENT_DISTRIB=bionic
SCRIPTS_PATH=$(dirname "$(realpath "$0")")
YANDEX_ARCHIVE_KEY_ID=7FCD11186050CD1A

ESSENTIAL_PACKAGES=" \
  ccache \
  clang-9 \
  clang-format-7 \
  cmake \
  git \
  libstdc++-7-dev \
  lld-9 \
  make \
  taxi-deps-py3-2 \
"

USERVER_BUILDDEPS=" \
  libboost-filesystem-dev \
  libboost-locale-dev \
  libboost-program-options-dev \
  libboost-regex-dev \
  libboost-thread-dev \
  libbrotli-dev \
  libcctz-dev \
  libcrypto++-dev \
  libev-dev \
  libfmt-dev \
  libhiredis-dev \
  libhttp-parser-dev \
  libkrb5-dev \
  libldap2-dev \
  libnghttp2-dev \
  libpq-dev \
  libssl-dev \
  libyaml-cpp-dev \
  libyandex-taxi-c-ares-dev \
  libyandex-taxi-curl4-openssl-dev \
  libyandex-taxi-grpc++-dev \
  libyandex-taxi-jemalloc-dev \
  libyandex-taxi-mongo-c-driver-dev \
  postgresql-server-dev-12 \
  yandex-taxi-protobuf-compiler-grpc \
  zlib1g-dev \
"

OPTIONAL_PACKAGES=" \
  mongodb \
  postgresql-12 \
  redis-server \
"

# issue sudo token to not remind of it in every message
if ! sudo -n /bin/true; then
  echo "This script runs most operations as a superuser, please provide password."
  sudo /bin/true
fi

# ensure we have Yandex archive key installed
if ! apt-key adv --list-keys "${YANDEX_ARCHIVE_KEY_ID}" >/dev/null 2>&1; then
  echo "Yandex APT GPG key is not installed!" >&2
  echo "Environment is not set up, refer to the docs https://wiki.yandex-team.ru/taxi/backend/howtostart/ for more info" >&2
  exit 1
fi

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
warn_held_packages ${ESSENTIAL_PACKAGES}
sudo apt install -y $(skip_held_packages ${ESSENTIAL_PACKAGES})

# TODO: use codegen to make package list

echo "Installing common build dependencies"

# install userver build dependencies
warn_held_packages ${USERVER_BUILDDEPS}
sudo apt install -y $(skip_held_packages ${USERVER_BUILDDEPS})

if ccache -p | grep max_size | grep -q default; then
  echo "Configuring ccache"
  ccache -M40G
fi

echo
echo "For better experience you might want to install packages:" ${OPTIONAL_PACKAGES} ", but they are optional."
