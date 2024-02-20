#!/bin/bash

# Exit on any error
set -eo pipefail

for REQUIREMENT in requirements/*.txt; do
  pip3 install -r ${REQUIREMENT}
done
