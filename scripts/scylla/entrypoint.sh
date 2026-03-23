#!/bin/bash
set -eox pipefail

(cd /opt/userver && ./scripts/scylla/local_build_and_install_all.sh)

cd /opt/userver/samples/scylla_service
rm -rf build
mkdir build && cd build
cmake .. -GNinja
ninja -j$(nproc)

./userver-samples-scylla_service --config ../static_config.yaml
