# Copyright 1999-2025 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

DESCRIPTION="Meta-package with dependencies for Production-ready C++ Asynchronous Framework with rich functionality."

HOMEPAGE="https://github.com/userver-framework/userver"

SRC_URI=""

S="${WORKDIR}"

LICENSE="Apache-2.0"

SLOT="0"

KEYWORDS="amd64 ~x86 ~arm ~arm64"

IUSE="grpc postgres redis mongodb mysql rabbitmq kafka rocksdb utest"

RDEPEND="!dev-cpp/userver dev-libs/cyrus-sasl[static-libs] dev-libs/protobuf app-crypt/mit-krb5 dev-libs/boost dev-cpp/yaml-cpp dev-debug/gdb dev-db/unixODBC[static-libs] dev-lang/python[ssl] dev-libs/crypto++[static-libs] dev-libs/jemalloc dev-libs/libbson[static-libs] dev-libs/libev[static-libs] dev-libs/libfmt dev-libs/openssl[static-libs] dev-libs/pugixml dev-libs/re2 dev-python/pip dev-python/voluptuous dev-util/ccache dev-build/cmake dev-build/ninja dev-vcs/git llvm-core/clang net-dns/c-ares[static-libs] net-libs/nghttp2 net-misc/curl[static-libs] net-nds/openldap[static-libs] sys-libs/zlib[static-libs] grpc? ( net-libs/grpc ) postgres? ( dev-db/postgresql[static-libs] ) redis? ( dev-db/redis dev-libs/hiredis[static-libs] ) mongodb? ( dev-db/mongodb dev-libs/mongo-c-driver[static-libs] ) mysql? ( dev-db/mariadb ) rabbitmq? ( dev-cpp/amqp-cpp ) kafka? ( dev-libs/librdkafka ) rocksdb? ( dev-libs/rocksdb[static-libs] ) utest? ( dev-cpp/gtest dev-cpp/benchmark )"

DEPEND="${RDEPEND}"

BDEPEND=""
