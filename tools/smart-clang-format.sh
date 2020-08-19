#!/bin/bash

# params
CLANG_FORMAT_VERSION=7
SOURCE_REGEXP="(^|\./)(core|mongo|postgresql|redis|shared|tests|examples|tools)/.+\.([ch]pp|inc)"

# check for GNU find and xargs
if find --version 2>/dev/null | grep -q GNU; then
  FIND="find . -regextype posix-egrep"
else
  FIND="find -E ."
fi

if xargs --version 2>/dev/null | grep -q GNU; then
  XARGS="xargs -r"
else
  XARGS="xargs"
fi

# check clang-format
CLANG_FORMAT=clang-format-$CLANG_FORMAT_VERSION
if ! [ -x `command -v $CLANG_FORMAT || echo ERROR` ]; then
  CLANG_FORMAT=clang-format
  if [ `$CLANG_FORMAT --version | grep -F "version $CLANG_FORMAT_VERSION" | wc -l` -ne 1 ]; then
    echo "Error: clang-format must be version $CLANG_FORMAT_VERSION" >&2
    exit 1
  fi
fi

cd $(dirname "$0")/../

case $1 in
  '') # only changed
  git diff HEAD --name-only --diff-filter=ACMR | \
  grep -E "$SOURCE_REGEXP$" | \
  $XARGS $CLANG_FORMAT -i
  ;;
  -a|--all) # all sources
  $FIND -type f -regex "$SOURCE_REGEXP" -exec \
  $CLANG_FORMAT -i {} +
  ;;
  *) # error
  echo "Error: $0 [-a|--all]" >&2
  exit 1
  ;;
esac
