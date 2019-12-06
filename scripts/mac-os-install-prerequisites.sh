#!/bin/sh

echo "Adding brew taxi-external tap"
brew tap taxi-external/tap https://github.yandex-team.ru/taxi-external/tap.git

echo "Adding brew tap for mongo (testsuite)"
brew tap mongodb/brew

echo "Update brew repos"
brew update

echo "Installing required packages with brew"
REQUIRED_PACKAGES="coreutils boost cctz cmake cryptopp curl flatbuffers fmt grpc hiredis http-parser jemalloc libev postgres openssl protobuf rapidjson yaml-cpp yandex-taxi-mongo-c-driver libiconv python"
brew install $REQUIRED_PACKAGES

# for tests and uservices
EXTRA_PACKAGES="catboost-model-lib ccache clang-format-7 cmake geobase6 libyandex-taxi-graph2 pugixml redis taxi-graph3-test-data ticket_parser2 mongodb-community@4.2 libyandex-taxi-v8 libpng"
brew install $EXTRA_PACKAGES

brew list jsoncpp > /dev/null 2>&1
HAS_JSON_CPP=$?
if [ $HAS_JSON_CPP -eq 0 ]; then
  echo "jsoncpp installed with brew is incompatible with userver (it has it's own). Please remove it"
fi
