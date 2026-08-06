#!/bin/bash

# Exit on any error and treat unset variables as errors
set -euo pipefail

# uv is used by userver to speed up Python virtual environment creation
python3 -m pip install uv

for REQUIREMENT in requirements/*.txt; do
  python3 -m pip install -r ${REQUIREMENT}
done
