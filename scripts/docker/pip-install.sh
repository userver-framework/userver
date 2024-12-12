#!/bin/bash

# Exit on any error and treat unset variables as errors
set -euo pipefail

for REQUIREMENT in requirements/*.txt; do
  pip3 install --break-system-packages -r ${REQUIREMENT}
done
