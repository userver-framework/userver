#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

DISTRO=${DISTRO:-ubuntu-24.04}
ALL_MODULES=$(ls scripts/docs/en/deps/$DISTRO)
ALL_PACKAGES=$(echo $ALL_MODULES | xargs -n1 | sed 's/.*/libuserver-\0-dev/')
BUILD_DEPENDENCIES=$(cat scripts/docs/en/deps/$DISTRO.md | sed 's/$/,/' | xargs | sed 's/,$//')


# Cleanup
rm -rf debian/


# Static files
mkdir -p debian/
cp scripts/debian-rules debian/rules


# Generate debian/control
cat >debian/control <<EOF
Source: userver
Section: libdevel
Priority: optional
Maintainer: Antony Polukhin <antoshkka@userver.tech>
Build-Depends: debhelper-compat (= 13),
               dh-cmake,
	       dh-cmake-compat (= 1),
               dh-sequence-ctest,
               dh-sequence-cpack,
               cmake (>= 3.14),
               pkg-config,
               libc6-dev,
	       $BUILD_DEPENDENCIES
Standards-Version: 4.6.2
Homepage: https://userver.tech
Vcs-Git: https://github.com/userver-framework/userver.git
Vcs-Browser: https://github.com/userver-framework/userver

Package: libuserver-all-dev
Architecture: any
Depends: $(echo $ALL_PACKAGES | sed 's/ /, /g' | xargs), libuserver-examples
Description:
 userver is the modern open source asynchronous framework with a rich set
 of abstractions for fast and comfortable creation of C++ microservices,
 services and utilities.
 .
 This metapackage provides the complete userver development environment,
 including all modules, libraries and examples.

Package: libuserver-examples
Architecture: any
Description:
 userver is the modern open source asynchronous framework with a rich set
 of abstractions for fast and comfortable creation of C++ microservices,
 services and utilities.
 .
 This package contains userver example services.

EOF

for MODULE in $ALL_MODULES; do
    echo "$MODULE" >debian/libuserver-$MODULE-dev.cpack-components

    cat >>debian/control <<EOF
Package: libuserver-$MODULE-dev
Architecture: any
Depends: \${cpack:Depends}, \${misc:Depends}
Description:
 userver is the modern open source asynchronous framework with a rich set
 of abstractions for fast and comfortable creation of C++ microservices,
 services and utilities.
 .
 This package provides $MODULE module/library support.

EOF
done


# TODO: parse ./scripts/docs/en/userver/roadmap_and_changelog.md ?
#
# Generate debian/changelog
cat >debian/changelog <<EOF
userver (1.0-1) UNRELEASED; urgency=medium

  * Initial release (Closes: #nnnn)  <nnnn is the bug number of your ITP>

 -- segoon <segoon@unknown>  Wed, 20 Aug 2025 22:19:05 +0300
EOF
