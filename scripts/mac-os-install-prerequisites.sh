#!/bin/sh

echo "Adding brew taxi-external tap"
brew tap taxi-external/tap https://github.yandex-team.ru/taxi-external/tap.git

echo "Adding brew tap for mongo (testsuite)"
brew tap mongodb/brew

echo "Update brew repos"
brew update

echo "Uninstalling old/conflicting formulae"
brew uninstall --ignore-dependencies grpc
brew uninstall --ignore-dependencies protobuf
brew uninstall homebrew/core/cryptopp

echo "Installing Python 3.7 with brew"
brew install python@3.7

# install symlinks only if they aren't there yet
HOMEBREW_ROOT=`brew config | grep 'HOMEBREW_PREFIX:' | cut -d' ' -f2`
if [ ! -e "${HOMEBREW_ROOT}/bin/python3.7" ]; then
  echo "Installing python3.7 symlinks"
  brew unlink python
  brew link --force python@3.7
  brew link --overwrite python
fi

echo "Installing required packages with brew"
REQUIRED_PACKAGES=" \
  boost \
  ccache \
  clang-format-7 \
  cmake \
  coreutils \
  cryptopp \
  curl \
  flatbuffers \
  fmt \
  git \
  grpc-python37 \
  hiredis \
  http-parser \
  jemalloc \
  libev \
  libiconv \
  openssl \
  postgres \
  protobuf-python37 \
  rapidjson \
  svn \
  yaml-cpp \
  yandex-taxi-mongo-c-driver \
"
brew install $REQUIRED_PACKAGES

if ccache -p | grep max_size | grep -q default; then
  echo "Configuring ccache"
  ccache -M40G
fi

# this has bad cmake file in bottle
brew install -s cctz

# for tests and uservices
EXTRA_PACKAGES=" \
  android-protector3-legacy \
  catboost-model-lib \
  icu4c \
  libpng \
  libyandex-taxi-graph \
  matrixnetmock \
  mongodb-community@4.2 \
  persqueue-wrapper \
  pugixml \
  redis \
  taxi-graph3-test-data \
  ticket_parser2 \
  yt-wrapper \
"
brew install $EXTRA_PACKAGES
brew install geobase6 --with-geodata

brew install libyandex-taxi-v8 # fails intermittently, install separately
if ! brew list --versions libyandex-taxi-v8 | grep -q .; then
  echo >&2 "libyandex-taxi-v8 installation failed (it does sometimes)."
  echo >&2 "If you need it, try installing it manually again with 'brew install libyandex-taxi-v8'."
fi

PYTHON_DEPS=" \
  pycryptodome \
  yandex-pgmigrate \
"
pip3.7 install $PYTHON_DEPS
