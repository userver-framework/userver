#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

# Preparing to add new repos
apt update
DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
  apt-transport-https ca-certificates dirmngr wget curl software-properties-common \
  gnupg gnupg2

gpg_retrieve_curl() {
  # See https://unix.stackexchange.com/questions/682929/migrating-away-from-apt-key-adv
  # and https://github.com/llvm/llvm-project/issues/55784#issuecomment-1569643347
  curl -fsSL "$1" | gpg --dearmor -o "/usr/share/keyrings/$2.gpg"
  chmod a+r "/usr/share/keyrings/$2.gpg"
}

gpg_retrieve_keyserver() {
  GNUPGHOME=$(mktemp -d)
  GNUPGHOME="$GNUPGHOME" gpg --no-default-keyring --keyring "/usr/share/keyrings/$2.gpg" --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys $1
  rm -rf "$GNUPGHOME"
  chmod a+r "/usr/share/keyrings/$2.gpg"
}


# Adding clang/llvm repos
gpg_retrieve_curl https://apt.llvm.org/llvm-snapshot.gpg.key llvm-snapshot
printf "\
deb [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-$(lsb_release -cs)-16 main \n\
deb-src [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-$(lsb_release -cs)-16 main\n\
deb [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-$(lsb_release -cs)-18 main \n\
deb-src [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-$(lsb_release -cs)-18 main\n" > /etc/apt/sources.list.d/llvm.list

# Adding Ubuntu toolchain repos
gpg_retrieve_keyserver 60C317803A41BA51845E371A1E9377A2BA9EF27F ubuntu-toolchain-r
printf "\
deb [signed-by=/usr/share/keyrings/ubuntu-toolchain-r.gpg] https://ppa.launchpadcontent.net/ubuntu-toolchain-r/test/ubuntu $(lsb_release -cs) main \n\
deb-src [signed-by=/usr/share/keyrings/ubuntu-toolchain-r.gpg] https://ppa.launchpadcontent.net/ubuntu-toolchain-r/test/ubuntu $(lsb_release -cs) main \n" \
  > /etc/apt/sources.list.d/ubuntu-toolchain-r.list

# Adding clickhouse repositories as in https://clickhouse.com/docs/en/install#setup-the-debian-repository
gpg_retrieve_keyserver 8919F6BD2B48D754 clickhouse-keyring
echo "deb [signed-by=/usr/share/keyrings/clickhouse-keyring.gpg] https://packages.clickhouse.com/deb stable main"  \
    | tee /etc/apt/sources.list.d/clickhouse.list

# Adding mariadb repositories (from https://www.linuxcapable.com/how-to-install-mariadb-on-ubuntu-linux/ )
gpg_retrieve_curl http://mirror.mariadb.org/PublicKey_v2 mariadb
# Restore the correct URL after https://jira.mariadb.org/browse/MDBF-651
#echo "deb [arch=amd64,arm64,ppc64el signed-by=/usr/share/keyrings/mariadb.gpg] https://deb.mariadb.org/10.11/ubuntu $(lsb_release -cs) main" \
#    | tee /etc/apt/sources.list.d/mariadb.list
echo "deb [arch=amd64,arm64,ppc64el signed-by=/usr/share/keyrings/mariadb.gpg] https://mirror.kumi.systems/mariadb/repo/10.11/ubuntu $(lsb_release -cs) main" \
    | tee /etc/apt/sources.list.d/mariadb.list

# Adding librdkafka confluent repositories as in https://docs.confluent.io/platform/current/installation/installing_cp/deb-ubuntu.html#get-the-software
gpg_retrieve_keyserver 8B1DA6120C2BF624 confluent
printf "\
deb [arch=amd64 signed-by=/usr/share/keyrings/confluent.gpg] https://packages.confluent.io/deb/7.6 stable main\n\
deb [signed-by=/usr/share/keyrings/confluent.gpg] https://packages.confluent.io/clients/deb $(lsb_release -cs) main\n" \
    | tee /etc/apt/sources.list.d/confluent.list

# convoluted setup of rabbitmq + erlang taken from https://www.rabbitmq.com/install-debian.html#apt-quick-start-packagecloud
## Team RabbitMQ's main signing key
gpg_retrieve_curl https://keys.openpgp.org/vks/v1/by-fingerprint/0A9AF2115F4687BD29803A206B73A36E6026DFCA com.rabbitmq.team
## Launchpad PPA that provides modern Erlang releases
gpg_retrieve_curl "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0xf77f1eda57ebb1cc" net.launchpad.ppa.rabbitmq.erlang
## PackageCloud RabbitMQ repository
gpg_retrieve_curl https://packagecloud.io/rabbitmq/rabbitmq-server/gpgkey io.packagecloud.rabbitmq
## Add apt repositories maintained by Team RabbitMQ
printf "\
deb [signed-by=/usr/share/keyrings/net.launchpad.ppa.rabbitmq.erlang.gpg] http://ppa.launchpad.net/rabbitmq/rabbitmq-erlang/ubuntu $(lsb_release -cs) main \n\
deb-src [signed-by=/usr/share/keyrings/net.launchpad.ppa.rabbitmq.erlang.gpg] http://ppa.launchpad.net/rabbitmq/rabbitmq-erlang/ubuntu $(lsb_release -cs) main \n\
deb [signed-by=/usr/share/keyrings/io.packagecloud.rabbitmq.gpg] https://packagecloud.io/rabbitmq/rabbitmq-server/ubuntu/ $(lsb_release -cs) main \n\
deb-src [signed-by=/usr/share/keyrings/io.packagecloud.rabbitmq.gpg] https://packagecloud.io/rabbitmq/rabbitmq-server/ubuntu/ $(lsb_release -cs) main\n" \
    | tee /etc/apt/sources.list.d/rabbitmq.list

gpg_retrieve_curl https://www.mongodb.org/static/pgp/server-6.0.asc mongodb
echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/6.0 multiverse" \
    | tee /etc/apt/sources.list.d/mongodb-org-6.0.list

# Some of the above repos are slow to respond or could be overloaded. Adding some retries
echo '
Acquire::Retries "20";
Acquire::https::Timeout "15";
Acquire::http::Timeout "15";
Acquire::ftp::Timeout "15";
' | tee /etc/apt/apt.conf.d/99custom_increase_retries

# Install build dependencies
if [ ! -f ubuntu-22.04.md ]; then
    echo "File 'ubuntu-22.04.md' not found!"
    exit 1
fi

PACKAGES_TO_INSTALL=$(cat ubuntu-22.04.md | tr '\n' ' ')
declare -a PACKAGES_FOR_MANUAL_INSTALL=(
  "postgresql-server-dev-14"
)
for PACKAGE in "${PACKAGES_FOR_MANUAL_INSTALL[@]}"; do
  PACKAGES_TO_INSTALL=( "${PACKAGES_TO_INSTALL[@]/$PACKAGE}" )
done

echo "PACKAGES_TO_INSTALL=${PACKAGES_TO_INSTALL}"

apt update

DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
  ${PACKAGES_TO_INSTALL} \
  chrpath \
  sudo \
  python3-pip \
  locales

# Installing postgresql-server-dev-14 without dependencies
#
# We should feel sorry because of that but we don't. The package
# pulls in ~700MB of unnecessary dependencies.
apt download postgresql-server-dev-14
dpkg-deb -R postgresql-server-dev-14* tmp_postgresql
cp -r tmp_postgresql/usr/* /usr/
rm -rf postgresql-server-dev-14* tmp_postgresql

# Cleanup
apt clean all

# You could override those versions from command line
AMQP_VERSION=${AMQP_VERSION:=v4.3.18}
CLICKHOUSE_VERSION=${CLICKHOUSE_VERSION:=v2.3.0}
ROCKSDB_VERSION=${ROCKSDB_VERSION:=v8.9.1}

# Installing amqp/rabbitmq client libraries from sources
git clone --depth 1 -b ${AMQP_VERSION} https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git amqp-cpp
(cd amqp-cpp && mkdir build && cd build && \
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install)

# Installing Clickhouse C++ client libraries from sources
git clone --depth 1 -b ${CLICKHOUSE_VERSION} https://github.com/ClickHouse/clickhouse-cpp.git
(cd clickhouse-cpp && mkdir build && cd build && \
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install)

# Installing RocksDB client libraries from sources
git clone --depth 1 -b ${ROCKSDB_VERSION} https://github.com/facebook/rocksdb
(cd rocksdb && mkdir build && cd build && \
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DROCKSDB_BUILD_SHARED=OFF -DWITH_TESTS=OFF -DWITH_BENCHMARK_TOOLS=OFF -DWITH_TOOLS=OFF -DUSE_RTTI=ON .. && make -j $(nproc) && make install)
(cd rocksdb && mkdir build-debug && cd build-debug && \
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DROCKSDB_BUILD_SHARED=OFF -DWITH_TESTS=OFF -DWITH_BENCHMARK_TOOLS=OFF -DWITH_TOOLS=OFF  -DUSE_RTTI=ON .. && make -j $(nproc) && make install)

# Installing Kafka
DEBIAN_FRONTEND=noninteractive apt install -y default-jre

curl https://dlcdn.apache.org/kafka/3.8.0/kafka_2.13-3.8.0.tgz -o kafka.tgz
mkdir -p /etc/kafka
tar xf kafka.tgz --directory=/etc/kafka
cp -r /etc/kafka/kafka_2.13-3.8.0/* /etc/kafka/
rm -rf /etc/kafka/kafka_2.13-3.8.0

# Set UTC timezone
TZ=Etc/UTC
ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Generate locales
locale-gen en_US.UTF-8
update-locale LC_ALL="en_US.UTF-8" LANG="en_US.UTF-8" LANGUAGE="en_US.UTF-8"
