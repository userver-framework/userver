# Copyright 1999-2025 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

DESCRIPTION="Production-ready C++ Asynchronous Framework with rich functionality."

HOMEPAGE="https://github.com/userver-framework/userver"

inherit pypi

MAIN_SRC="v2.13.tar.gz"

PYTHON_LIBS=(
  "wheel 0.45.1"
  "typing-extensions 4.15.0"
  "sqlparse 0.5.4"
  "redis 7.1.0"
  "PyMySQL 1.1.2"
  "pygments 2.19.2"
  "propcache 0.4.1"
  "pluggy 1.6.0"
  "packaging 25.0"
  "multidict 6.7.0"
  "iniconfig 2.3.0"
  "idna 3.11"
  "frozenlist 1.8.0"
  "dnspython 2.8.0"
  "crc 7.1.0"
  "cached-property 2.0.1"
  "attrs 25.4.0"
  "async-timeout 5.0.1"
  "aiohappyeyeballs 2.6.1"
  "yarl 1.22.0"
  "yandex-pgmigrate 1.0.10"
  "pytest 9.0.1"
  "aiosignal 1.4.0"
# "aiokafka 0.12.0" (pypi mistake)
  "pytest-asyncio 1.3.0"
  "aiormq 6.9.2"
  "pytest-aiohttp 1.1.0"
  "aio-pika 9.5.8"
  "yandex-taxi-testsuite 0.3.9"
  "Jinja2 3.1.6"
  "pydantic 2.12.5"
  "setuptools 80.9.0"
  "annotated-types 0.7.0"
  "typing-inspection 0.4.2"
)

PY2PY3_LIBS=(
  "six 1.17.0"
  "py 1.11.0"
  "pamqp 3.3.0"
  "python-dateutil 2.9.0.post0"
)

MANYLINUX_PYTHON_LIBS=(
  "pymongo 4.15.5"
  "pyyaml 6.0.3"
  "aiohttp 3.13.2"
# "pydantic_core 2.41.5" (pypi bug; download url: https://files.pythonhosted.org/packages/cf/4e/35a80cae583a37cf15604b44240e45c05e04e86f9cfd766623149297e971/pydantic_core-2.41.5-cp313-cp313-manylinux_2_17_x86_64.manylinux2014_x86_64.whl )
  "MarkupSafe 3.0.3"
)

PYTHON_SDIST_LIBS=(
  "psycopg2 2.9.11"
  "python-redis 0.4.0"
)

CPM_LIBS="main.zip"

CPM_LIBS_TAR="userver_cpm_cache.tar.gz"

PATCH="e1e4a07a5d281fcd39281d3b95981b6ce6749db6.zip"

SRC_URI="https://github.com/userver-framework/userver/archive/refs/tags/${MAIN_SRC} https://github.com/halfdarkangel/userver-cpm-cache/archive/refs/heads/${CPM_LIBS} https://gist.github.com/halfdarkangel/83f00a44f289e63c3bc7fe30d844d202/archive/${PATCH} https://files.pythonhosted.org/packages/cf/4e/35a80cae583a37cf15604b44240e45c05e04e86f9cfd766623149297e971/pydantic_core-2.41.5-cp313-cp313-manylinux_2_17_x86_64.manylinux2014_x86_64.whl"

for dep in "${PYTHON_LIBS[@]}"; do
	set -- ${dep}
	name=$1
	ver=$2
	SRC_URI+=" $(pypi_wheel_url "${name}" "${ver}")"
done

for dep in "${PY2PY3_LIBS[@]}"; do
        set -- ${dep}
        name=$1
        ver=$2
	SRC_URI+=" $(pypi_wheel_url "${name}" "${ver}" "py2.py3")"
done

for dep in "${MANYLINUX_PYTHON_LIBS[@]}"; do
        set -- ${dep}
        name=$1
        ver=$2
        SRC_URI+=" $(pypi_wheel_url "${name}" "${ver}" "cp313" "cp313-manylinux2014_x86_64.manylinux_2_17_x86_64.manylinux_2_28_x86_64")"
done

for dep in "${PYTHON_SDIST_LIBS[@]}"; do
        set -- ${dep}
        name=$1
        ver=$2
        SRC_URI+=" $(pypi_sdist_url "${name}" "${ver}")"
done

SRC_URI+=" $(pypi_wheel_url aiokafka 0.12.0 cp313 cp313-manylinux_2_17_x86_64.manylinux2014_x86_64)"

S="${WORKDIR}/${P}"

CATEGORY="dev-cpp"

LICENSE="Apache-2.0"

SLOT="0"

KEYWORDS="amd64"

#IUSE="chaotic grpc grpc-reflection postgres redis redistls mongodb mysql clickhouse rabbitmq kafka rocksdb opentelemetry s3api ydb utest testsute easy"

IUSE="chaotic postgres redis mongodb mysql rabbitmq kafka utest testsute easy"

#REQUIRED_USE=(
#	"grpc-reflection? ( grpc )"
#	"redistls? ( redis )"
#	"s3api? (grpc)"
#	"opentelemetry? (grpc)"
#	)

#RDEPEND="!dev-cpp/userver-meta dev-libs/cyrus-sasl[static-libs] dev-libs/protobuf app-crypt/mit-krb5 dev-libs/boost dev-cpp/yaml-cpp dev-debug/gdb dev-db/unixODBC[static-libs] dev-lang/python[ssl] dev-libs/crypto++[static-libs] dev-libs/jemalloc dev-libs/libbson[static-libs] dev-libs/libev[static-libs] dev-libs/libfmt dev-libs/openssl[static-libs] dev-libs/pugixml dev-libs/re2 dev-python/pip dev-python/voluptuous dev-util/ccache dev-build/cmake dev-build/ninja dev-vcs/git llvm-core/clang net-dns/c-ares[static-libs] net-libs/nghttp2 net-misc/curl[static-libs] net-nds/openldap[static-libs] sys-libs/zlib[static-libs] grpc? ( net-libs/grpc ) postgres? ( dev-db/postgresql[static-libs] ) redis? ( dev-db/redis dev-libs/hiredis[static-libs] ) mongodb? ( dev-db/mongodb dev-libs/mongo-c-driver[static-libs] ) mysql? ( dev-db/mariadb ) rabbitmq? ( dev-cpp/amqp-cpp ) kafka? ( dev-libs/librdkafka ) rocksdb? ( dev-libs/rocksdb[static-libs] ) utest? ( dev-cpp/gtest dev-cpp/benchmark )"

RDEPEND="!dev-cpp/userver-meta dev-libs/cyrus-sasl[static-libs] dev-libs/protobuf app-crypt/mit-krb5 dev-libs/boost dev-cpp/yaml-cpp dev-debug/gdb dev-db/unixODBC[static-libs] dev-lang/python[ssl] dev-libs/crypto++[static-libs] dev-libs/jemalloc dev-libs/libbson[static-libs] dev-libs/libev[static-libs] dev-libs/libfmt dev-libs/openssl[static-libs] dev-libs/pugixml dev-libs/re2 dev-python/pip dev-python/voluptuous dev-util/ccache dev-build/cmake dev-build/ninja dev-vcs/git llvm-core/clang net-dns/c-ares[static-libs] net-libs/nghttp2 net-misc/curl[static-libs] net-nds/openldap[static-libs] sys-libs/zlib[static-libs] postgres? ( dev-db/postgresql[static-libs] ) redis? ( dev-db/redis dev-libs/hiredis[static-libs] ) mongodb? ( dev-db/mongodb dev-libs/mongo-c-driver[static-libs] ) mysql? ( dev-db/mariadb ) rabbitmq? ( dev-cpp/amqp-cpp ) kafka? ( dev-libs/librdkafka ) utest? ( dev-cpp/gtest dev-cpp/benchmark )"

DEPEND="${RDEPEND}"

BDEPEND="app-arch/unzip"

src_unpack(){
	unpack "${DISTDIR}/${MAIN_SRC}" || die "unable to unpack source code"
	cd "${S}" || die "unable to enter source code directory"
	unzip -o -j "${DISTDIR}/${PATCH}" -d "${S}" || die "unable to unpack pq patch"
	unzip -o -j "${DISTDIR}/${CPM_LIBS}" -d "${S}/third_party/" || die "unable to unpack cpm cache"
	cd "${S}/third_party" || die "unable to enter third-party directory"
	tar -xzf "${S}/third_party/${CPM_LIBS_TAR}" || die "unable to unpack cpm cache"
	mkdir "wheelhouse" || die "unable to create directory for python packages"
	for file in ${A}; do
		if [[ ${file} != ${MAIN_SRC} ]] && [[ ${file} != ${CPM_LIBS} ]] && [[ ${file} != ${PATCH} ]]; then
			cp "${DISTDIR}/${file}" "${S}/third_party/wheelhouse" || die "unable to copy ${file} to wheelhouse directory"
		fi
	done
}

src_prepare(){
	cd "${S}" || die "unable to enter build directory"
	eapply "${S}/libpq.patch" || die "unable to apply pq patch"
	eapply_user
}

src_configure() {
	cmake -S${S}/ -B build_debug -DCMAKE_BUILD_TYPE=Debug -DUSERVER_INSTALL=ON -DUSERVER_SANITIZE="ub addr" -DCPM_SOURCE_CACHE="${S}/third_party/cpm_cache" -DUSERVER_PIP_OPTIONS="--no-index;--find-links=${S}/third_party/wheelhouse" -DUSERVER_FEATURE_STACKTRACE=OFF -DUSERVER_FEATURE_CHAOTIC=$(usex chaotic) -DUSERVER_FEATURE_GRPC=OFF -DUSERVER_FEATURE_GRPC_REFLECTION=OFF -DUSERVER_FEATURE_POSTGRESQL=$(usex postgres) -DUSERVER_FEATURE_REDIS=$(usex redis) -DUSERVER_FEATURE_REDIS_TLS=OFF -DUSERVER_FEATURE_MONGODB=$(usex mongodb) -DUSERVER_FEATURE_MYSQL=$(usex mysql) -DUSERVER_FEATURE_CLICKHOUSE=OFF -DUSERVER_FEATURE_RABBITMQ=$(usex rabbitmq) -DUSERVER_FEATURE_KAFKA=$(usex kafka) -DUSERVER_FEATURE_ROCKS=OFF -DUSERVER_FEATURE_OTLP=OFF -DUSERVER_FEATURE_S3API=OFF -DUSERVER_FEATURE_YDB=OFF -DUSERVER_FEATURE_UTEST=$(usex utest) -DUSERVER_FEATURE_TESTSUITE=$(usex testsute) -DUSERVER_FEATURE_EASY=$(usex easy) -GNinja || die "unable to configure release version"

	cmake -S${S}/ -B build_release -DCMAKE_BUILD_TYPE=Release -DUSERVER_INSTALL=ON -DUSERVER_SANITIZE="" -DCPM_SOURCE_CACHE="${S}/third_party/cpm_cache" -DUSERVER_PIP_OPTIONS="--no-index;--find-links=${S}/third_party/wheelhouse" -DUSERVER_FEATURE_STACKTRACE=OFF -DUSERVER_FEATURE_CHAOTIC=$(usex chaotic) -DUSERVER_FEATURE_GRPC=OFF -DUSERVER_FEATURE_GRPC_REFLECTION=OFF -DUSERVER_FEATURE_POSTGRESQL=$(usex postgres) -DUSERVER_FEATURE_REDIS=$(usex redis) -DUSERVER_FEATURE_REDIS_TLS=OFF -DUSERVER_FEATURE_MONGODB=$(usex mongodb) -DUSERVER_FEATURE_MYSQL=$(usex mysql) -DUSERVER_FEATURE_CLICKHOUSE=OFF -DUSERVER_FEATURE_RABBITMQ=$(usex rabbitmq) -DUSERVER_FEATURE_KAFKA=$(usex kafka) -DUSERVER_FEATURE_ROCKS=OFF -DUSERVER_FEATURE_OTLP=OFF -DUSERVER_FEATURE_S3API=OFF -DUSERVER_FEATURE_YDB=OFF -DUSERVER_FEATURE_UTEST=$(usex utest) -DUSERVER_FEATURE_TESTSUITE=$(usex testsute) -DUSERVER_FEATURE_EASY=$(usex easy) -GNinja || die "unable to configure release version"
}

src_compile() {
	cmake --build "${S}/build_debug" -- -j$(nproc) || die "unable to build debug version"
	cmake --build "${S}/build_release" -- -j$(nproc) || die "unable to build release version"
}

src_install() {
	dodir /usr
	cp -a "${S}/build_debug/." "${D}/usr" || die "unable to install"
	cp -a "${S}/build_release/." "${D}/usr" || die "unable to install"
}

pkg_postinst(){
	rm -rf "${S}/build_debug/" "${S}/build_release/"
	ccache --clear
}
