#!/bin/bash

# Exit on any error and treat unset variables as errors
set -euo pipefail

# Preparing to add new repos
apt update 
DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
  apt-transport-https ca-certificates dirmngr wget curl software-properties-common \
  gnupg gnupg2

gpg_retrieve() {
  # See https://unix.stackexchange.com/questions/682929/migrating-away-from-apt-key-adv
  # and https://github.com/llvm/llvm-project/issues/55784#issuecomment-1569643347
  curl -fsSL "$1" | gpg --dearmor -o "/usr/share/keyrings/$2.gpg"
  chmod a+r "/usr/share/keyrings/$2.gpg"
}


# Adding clang/llvm repos
gpg_retrieve https://apt.llvm.org/llvm-snapshot.gpg.key llvm-snapshot
printf "\
deb [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main \n\
deb-src [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main\n" > /etc/apt/sources.list.d/llvm.list

# Adding GCC repos
add-apt-repository -y ppa:ubuntu-toolchain-r/test

# Adding clickhouse repositories as in https://clickhouse.com/docs/en/install#setup-the-debian-repository
GNUPGHOME=$(mktemp -d)
GNUPGHOME="$GNUPGHOME" gpg --no-default-keyring --keyring /usr/share/keyrings/clickhouse-keyring.gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 8919F6BD2B48D754
rm -rf "$GNUPGHOME"
chmod +r /usr/share/keyrings/clickhouse-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/clickhouse-keyring.gpg] https://packages.clickhouse.com/deb stable main"  \
    | tee /etc/apt/sources.list.d/clickhouse.list

# Adding mariadb repositories (from https://www.linuxcapable.com/how-to-install-mariadb-on-ubuntu-linux/ )
gpg_retrieve http://mirror.mariadb.org/PublicKey_v2 mariadb
echo "deb [arch=amd64,arm64,ppc64el signed-by=/usr/share/keyrings/mariadb.gpg] http://mirror.mariadb.org/repo/10.11/ubuntu/ $(lsb_release -cs) main" \
    | tee /etc/apt/sources.list.d/mariadb.list

# convoluted setup of rabbitmq + erlang taken from https://www.rabbitmq.com/install-debian.html#apt-quick-start-packagecloud
## Team RabbitMQ's main signing key
gpg_retrieve https://keys.openpgp.org/vks/v1/by-fingerprint/0A9AF2115F4687BD29803A206B73A36E6026DFCA com.rabbitmq.team
## Launchpad PPA that provides modern Erlang releases
gpg_retrieve "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0xf77f1eda57ebb1cc" net.launchpad.ppa.rabbitmq.erlang
## PackageCloud RabbitMQ repository
gpg_retrieve https://packagecloud.io/rabbitmq/rabbitmq-server/gpgkey io.packagecloud.rabbitmq
## Add apt repositories maintained by Team RabbitMQ
printf "\
deb [signed-by=/usr/share/keyrings/net.launchpad.ppa.rabbitmq.erlang.gpg] http://ppa.launchpad.net/rabbitmq/rabbitmq-erlang/ubuntu focal main \n\
deb-src [signed-by=/usr/share/keyrings/net.launchpad.ppa.rabbitmq.erlang.gpg] http://ppa.launchpad.net/rabbitmq/rabbitmq-erlang/ubuntu focal main \n\
deb [signed-by=/usr/share/keyrings/io.packagecloud.rabbitmq.gpg] https://packagecloud.io/rabbitmq/rabbitmq-server/ubuntu/ focal main \n\
deb-src [signed-by=/usr/share/keyrings/io.packagecloud.rabbitmq.gpg] https://packagecloud.io/rabbitmq/rabbitmq-server/ubuntu/ focal main\n" \
    | tee /etc/apt/sources.list.d/rabbitmq.list

gpg_retrieve https://www.mongodb.org/static/pgp/server-6.0.asc mongodb
echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/6.0 multiverse" \
    | tee /etc/apt/sources.list.d/mongodb-org-6.0.list

# Install build dependencies
if [ ! -f ubuntu-22.04.md ]; then
    echo "File 'ubuntu-22.04.md' not found!"
    exit 1
fi

PACKAGES_TO_INSTALL=$(cat ubuntu-22.04.md | tr '\n' ' ')
declare -a PACKAGES_FOR_MANUAL_INSTALL=(
  "postgresql-server-dev-14"
  "redis-server"
  "postgresql-14"
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

# Set UTC timezone
TZ=Etc/UTC
ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Generate locales
locale-gen en_US.UTF-8
update-locale LC_ALL="en_US.UTF-8" LANG="en_US.UTF-8" LANGUAGE="en_US.UTF-8"
