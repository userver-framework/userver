#!/bin/sh

echo "Adding brew taxi-external tap"
brew tap taxi-external/tap https://github.yandex-team.ru/taxi-external/tap.git

echo "Installing required packages with brew"
REQUIRED_PACKAGES="cppcheck jemalloc libev http-parser yandex-taxi-mongo-c-driver yaml-cpp hiredis postgres openssl cryptopp cctz libyandex-taxi-graph"

brew install boost
brew install $REQUIRED_PACKAGES

brew list jsoncpp > /dev/null 2>&1
HAS_JSON_CPP=$?
if [ $HAS_JSON_CPP -eq 0 ]; then
  echo "jsoncpp installed with brew is incompatible with userver (it has it's own). Please remove it"
fi
