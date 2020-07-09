#!/bin/sh

echo "Adding brew taxi-external tap"
brew tap taxi-external/tap https://github.yandex-team.ru/taxi-external/tap.git

echo "Adding brew tap for mongo (testsuite)"
brew tap mongodb/brew

echo "Update brew repos"
brew update

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
  grpc \
  hiredis \
  http-parser \
  jemalloc \
  libev \
  libiconv \
  openssl \
  postgres \
  protobuf \
  python \
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
  libyandex-taxi-v8 \
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

PYTHON_DEPS=" \
  pycryptodome \
  yandex-pgmigrate \
"
pip3 install $PYTHON_DEPS
