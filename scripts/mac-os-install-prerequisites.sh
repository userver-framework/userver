#!/bin/sh

ensure_ssh_agent() {
  ssh-add -l >/dev/null && return

  if [ $? -eq 1 ]; then
    echo "No keys are loaded into ssh-agent, running ssh-add for you:"
    ssh-add && return
  fi

  echo "SSH agent is not configured properly, some packages may fail to install"
  echo
  sleep 2
}

echo "Checking SSH agent availability"
ensure_ssh_agent

echo "Checking arcadia connectivity"
ssh -q arcadia.yandex.ru >/dev/null
if [ $? -eq 255 ]; then
  echo "Connection check failed, some packages may fail to install"
  echo
  sleep 2
fi

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
brew uninstall mongodb-community-shell

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
  clang-format-9 \
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
  persqueue-wrapper \
  pugixml \
  redis \
  taxi-graph3-test-data \
  ticket_parser2 \
  yt-wrapper \
"
brew install $EXTRA_PACKAGES
brew install geobase6 --with-geodata

brew install mongodb-community@4.2
brew link --force mongodb-community@4.2

brew install libyandex-taxi-v8 # fails intermittently, install separately
if ! brew list --versions libyandex-taxi-v8 | grep -q .; then
  echo >&2 "libyandex-taxi-v8 installation failed (it does sometimes)."
  echo >&2 "If you need it, try installing it manually again with 'brew install libyandex-taxi-v8'."
fi

# python.org interpreter cannot build psycopg2 without it
export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/opt/openssl/lib/

if [[ "$VIRTUAL_ENV" = "" ]]
then
  PIP_FLAGS="--user"
else
  PIP_FLAGS=""
fi

PYTHON_DEPS=" \
  pycryptodome \
  yandex-pgmigrate \
"
pip3.7 install $PIP_FLAGS -i https://pypi.yandex-team.ru/simple/ $PYTHON_DEPS
