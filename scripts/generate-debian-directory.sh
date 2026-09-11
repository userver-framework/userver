#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euo pipefail

VERSION=${VERSION:-$(cat version.txt)}
VERSION=${VERSION//-/\~}
DISTRO=${DISTRO:-ubuntu-24.04}

# Modules whose C++ client libraries are unavailable in a suitable version and
# whose CMake feature flag is set to OFF in scripts/debian-rules.
# Keep this list in sync with the *_FEATURE_*=OFF flags in scripts/debian-rules,
# including distro-specific flags.
DISABLED_MODULES="clickhouse rabbitmq ydb"

case "$DISTRO" in
    ubuntu-22.04)
        SUITE=jammy
        DISABLED_MODULES+=" kafka grpc grpc-reflection otlp"
        ;;
    ubuntu-24.04) SUITE=noble ;;
    ubuntu-26.04) SUITE=resolute ;;
    *)
        echo "ERROR: unsupported DISTRO '$DISTRO'; PPA upload targets Ubuntu only." >&2
        echo "Supported: ubuntu-22.04 (jammy), ubuntu-24.04 (noble), ubuntu-26.04 (resolute)." >&2
        exit 1
        ;;
esac

# Map distros that need vendored wheels to their target CPython ABI.
PYVER=
case "$DISTRO" in
    ubuntu-22.04)
        PYVER=3.10
        PKG_CONFIG_PACKAGE=pkg-config
        ;;
    ubuntu-24.04)
        PYVER=3.12
        PKG_CONFIG_PACKAGE=pkg-config
        ;;
    ubuntu-26.04) PKG_CONFIG_PACKAGE=pkgconf ;;
esac

# pinned to main HEAD commit 3332dec5 (2022-08-01).
# WARNING: the auto-generated 1.xx.0 release tags (e.g. 1.50.0) do NOT track the
# main branch content and are missing google::rpc::ErrorInfo from error_details.proto.
# Upstream userver docker uses floating 'main'; we pin the commit for reproducibility.
# main is effectively frozen (README states protos are no longer updated here).
API_COMMON_PROTOS_COMMIT=3332dec527759859840a3a2ff108c67a54708130
OPENTELEMETRY_PROTO_TAG=v1.3.2

# 'examples' has a deps file (for ONE_PER_GROUP future-proofing) but its debian package
# is named 'libuserver-examples' (not '-dev') and is declared explicitly below, so exclude
# it from the standard module loop that generates 'libuserver-<m>-dev' packages.
SPECIAL_MODULES="examples"
ALL_MODULES=$(ls scripts/docs/en/deps/$DISTRO | grep -vxFf <(printf '%s\n' $DISABLED_MODULES $SPECIAL_MODULES))
ALL_PACKAGES=$(echo $ALL_MODULES | xargs -n1 | sed 's/.*/libuserver-\0-dev/')
BUILD_DEPENDENCIES=$(sed '/^build-essential$/d; s/$/,/' scripts/docs/en/deps/$DISTRO.md | xargs | sed 's/,$//')


rm -rf debian/
mkdir -p debian/source
cat >debian/source/format <<'EOF'
3.0 (native)
EOF

# Generate tar-ignore entries from .gitignore to keep a single source of truth.
{
    echo "# Auto-generated from .gitignore by generate-debian-directory.sh. Do not edit."
    while IFS= read -r line; do
        # Skip blank lines and comments
        [[ -z "$line" || "$line" == \#* ]] && continue
        # Skip negation patterns
        [[ "$line" == \!* ]] && continue
        # Strip trailing slash
        line="${line%/}"
        # Skip debian/ (must ship in source tarball)
        [[ "$line" == "/debian" || "$line" == "debian" ]] && continue
        # Preserve root-anchored .gitignore patterns for tar exclusions.
        if [[ "$line" == /* ]]; then
            line=".${line}"
        fi
        echo "tar-ignore = \"$line\""
    done < .gitignore
} >debian/source/options

cat >debian/source/lintian-overrides <<'EOF'
# The JSON-licensed jsonchecker sources are not vendored, but RapidJSON's license notice must be retained.
userver source: license-problem-json-evil [third_party/rapidjson/license.txt]
# This intentionally minified HTML file is the source fixture used by the benchmark service.
userver source: source-is-missing [samples/benchmark_service/data/static/footer.html]
EOF

# Jammy and Noble lack a sufficiently recent Pydantic package, and Jammy's
# PyYAML is also too old. Vendor wheels because Launchpad builders cannot reach
# PyPI. Resolute uses its native Pydantic 2 and PyYAML packages instead.
if [ -n "$PYVER" ]; then
    PYABI="cp${PYVER//./}"
    WHEEL_DIR=debian/vendor-wheels
    mkdir -p "$WHEEL_DIR"
    echo "Downloading Python wheels for ${DISTRO} (Python ${PYVER}, ${PYABI}, manylinux2014_x86_64)..."
    python3 -m pip download \
        'pydantic>=2.5.3,<3' \
        'PyYAML>=6.0.1' \
        --only-binary=:all: \
        --implementation cp \
        --python-version "$PYVER" \
        --abi "$PYABI" \
        --platform manylinux2014_x86_64 \
        -d "$WHEEL_DIR"
    echo "Vendored wheels:"
    ls "$WHEEL_DIR"
fi

# Vendor proto sources (offline Launchpad builder cannot reach GitHub).
PROTO_DIR=debian/vendor-protos
mkdir -p "$PROTO_DIR"

echo "Cloning api-common-protos at commit ${API_COMMON_PROTOS_COMMIT}..."
git clone --filter=blob:none --no-checkout \
    https://github.com/googleapis/api-common-protos.git \
    "$PROTO_DIR/api-common-protos"
AISUITE_ALLOW_GIT=1 git -C "$PROTO_DIR/api-common-protos" checkout "$API_COMMON_PROTOS_COMMIT"
rm -rf "$PROTO_DIR/api-common-protos/.git"

echo "Cloning opentelemetry-proto ${OPENTELEMETRY_PROTO_TAG}..."
git clone --depth 1 --branch "$OPENTELEMETRY_PROTO_TAG" \
    https://github.com/open-telemetry/opentelemetry-proto.git \
    "$PROTO_DIR/opentelemetry-proto"
rm -rf "$PROTO_DIR/opentelemetry-proto/.git"

echo "Vendored proto sources:"
ls "$PROTO_DIR"



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
	       $PKG_CONFIG_PACKAGE,
	       $BUILD_DEPENDENCIES
Standards-Version: 4.6.2
Homepage: https://userver.tech
Vcs-Git: https://github.com/userver-framework/userver.git
Vcs-Browser: https://github.com/userver-framework/userver

Package: libuserver-all-dev
Architecture: any
Depends: $(echo $ALL_PACKAGES | sed 's/ /, /g' | xargs), libuserver-examples, \${misc:Depends}
Description:
 userver is the modern open source asynchronous framework with a rich set
 of abstractions for fast and comfortable creation of C++ microservices,
 services and utilities.
 .
 This metapackage provides the complete userver development environment,
 including all modules, libraries and examples.

Package: libuserver-examples
Architecture: any
Depends: \${misc:Depends}
Description:
 userver is the modern open source asynchronous framework with a rich set
 of abstractions for fast and comfortable creation of C++ microservices,
 services and utilities.
 .
 This package contains userver example services.

EOF

for MODULE in $ALL_MODULES; do
    echo "$MODULE" >debian/libuserver-$MODULE-dev.cpack-components

    # Read the module's apt library deps (full content, intentionally including
    # build tools like clang-format/protobuf-compiler that appear in some modules —
    # heavier but simpler than curating). Empty for header-only extras (easy, otlp, …).
    MODULE_APT_DEPS=$(sed '/^\s*\(#\|$\)/d' "scripts/docs/en/deps/$DISTRO/$MODULE" 2>/dev/null \
        | tr '\n' ',' | sed 's/,$//' | sed 's/,/, /g')

    MODULE_DEPENDS="\${cpack:Depends}"
    if [ "$MODULE" = chaotic ]; then
        MODULE_DEPENDS="$MODULE_DEPENDS, \${shlibs:Depends}"
    fi
    MODULE_DEPENDS="$MODULE_DEPENDS, \${misc:Depends}"
    if [ -n "$MODULE_APT_DEPS" ]; then
        MODULE_DEPENDS="$MODULE_DEPENDS, $MODULE_APT_DEPS"
    fi

    cat >>debian/control <<EOF
Package: libuserver-$MODULE-dev
Architecture: any
Depends: $MODULE_DEPENDS
Description:
 userver is the modern open source asynchronous framework with a rich set
 of abstractions for fast and comfortable creation of C++ microservices,
 services and utilities.
 .
 This package provides $MODULE module/library support.

EOF
done

# The manylinux wheels link to the dynamic loader, libc.so.6, libdl.so.2,
# libgcc_s.so.1, libpthread.so.0, and librt.so.1. In merged-/usr environments,
# dpkg-shlibdeps may resolve them through diversions and fail to map them back
# to Ubuntu packages. Keep scanning the binaries, but provide the mappings.
cat >debian/shlibs.local <<EOF
ld-linux-x86-64 2 libc6
libc 6 libc6
libdl 2 libc6
libgcc_s 1 libgcc-s1
libpthread 0 libc6
librt 1 libc6
EOF

# 'examples' component is installed by USERVER_BUILD_SAMPLES=ON; its package is
# 'libuserver-examples' (non-dev, sample sources only). Write its .cpack-components
# so dh_cpack_install claims the files and dh_missing doesn't abort.
echo "examples" >debian/libuserver-examples.cpack-components

cp scripts/debian-copyright debian/copyright
cp scripts/debian-rules debian/rules

cat >debian/changelog <<EOF
userver ($VERSION) $SUITE; urgency=medium

  $(awk '/### Release/ {p++;} {if(p==1) print "  "$0;}' <scripts/docs/en/userver/roadmap_and_changelog.md)

 -- Vasily Kulikov <segoon@yandex-team.ru>  Wed, 20 Aug 2025 22:19:05 +0300
EOF

cat <<EOF
debian/{control,changelog,*} are generated.
Now run 'dpkg-buildpackage -S' to build debian source package.
EOF
