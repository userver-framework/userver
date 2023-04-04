# main system for docker
FROM ubuntu:22.04

# Set UTC timezone
ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

ENV DEBIAN_FRONTEND noninteractive

# install main packages
RUN apt-get update && apt-get install -y --allow-unauthenticated \
    binutils-dev \
    build-essential \
    ccache \
    chrpath \
    clang \
    clang-format \
    clang-tidy \
	cmake \
	libboost1.74-dev \
	libboost-program-options1.74-dev \
	libboost-filesystem1.74-dev \
	libboost-locale1.74-dev \
	libboost-regex1.74-dev \
	libboost-iostreams1.74-dev \
	libev-dev \
	zlib1g-dev \
	libcurl4-openssl-dev \
	curl \
	libyaml-cpp-dev \
	libssl-dev \
	libfmt-dev \
	libcctz-dev \
	libhttp-parser-dev \
	libnghttp2-dev \
	libjemalloc-dev \
	libmongoc-dev \
	libbson-dev \
	libldap2-dev \
	libpq-dev \
	postgresql-server-dev-all \
	libkrb5-dev \
	libhiredis-dev \
	python3-dev \
	python3-protobuf \
	python3-jinja2 \
	python3-virtualenv \
	virtualenv \
	python3-voluptuous \
	python3-yaml \
	libc-ares-dev \
	libspdlog-dev \
	libbenchmark-dev \
	libgmock-dev \
	libgtest-dev \
	ccache \
	git \
	postgresql \
	redis-server \
	vim \
	sudo \
	gnupg \
	gnupg2 \
	wget \
	dirmngr \
	libcrypto++-dev \
	libabsl-dev \
	liblz4-dev

RUN apt-get clean all


# add clickhouse repositories
RUN sudo apt-get install -y apt-transport-https ca-certificates dirmngr
RUN sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 8919F6BD2B48D754

RUN echo "deb https://packages.clickhouse.com/deb stable main" | sudo tee \
    /etc/apt/sources.list.d/clickhouse.list

# add mongodb repositories
RUN wget -qO - https://www.mongodb.org/static/pgp/server-5.0.asc | sudo apt-key add -

RUN echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/5.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-5.0.list

RUN echo "deb http://security.ubuntu.com/ubuntu focal-security main" | sudo tee /etc/apt/sources.list.d/focal-security.list

RUN apt-get update && \
    apt-get -o Dpkg::Options::="--force-confold" upgrade -q -y --force-yes && \
    apt-get -o Dpkg::Options::="--force-confold" dist-upgrade -q -y --force-yes

RUN apt update && apt install libssl1.1

RUN apt-get install -y --allow-unauthenticated \
    mongodb-org \
	mongodb-org-database \
	mongodb-org-server \
	mongodb-org-shell \
	mongodb-org-mongos \
	mongodb-org-tools \
	postgresql-common \
	locales \
	clickhouse-server \
	clickhouse-client

# add mariadb repositories
RUN wget https://downloads.mariadb.com/MariaDB/mariadb_repo_setup
RUN chmod +x mariadb_repo_setup
RUN ./mariadb_repo_setup

RUN apt-get install -y libmariadb-dev mariadb-server

# Generating locale
RUN sed -i 's/^# *\(en_US.UTF-8\)/\1/' /etc/locale.gen
RUN echo "export LC_ALL=en_US.UTF-8" >> ~/.bashrc
RUN echo "export LANG=en_US.UTF-8" >> ~/.bashrc
RUN echo "export LANGUAGE=en_US.UTF-8" >> ~/.bashrc

RUN locale-gen ru_RU.UTF-8
RUN locale-gen en_US.UTF-8
RUN echo LANG=en_US.UTF-8 >> /etc/default/locale

RUN mkdir -p /home/user
RUN chmod 777 /home/user

RUN pip3 install pep8

# convoluted setup of rabbitmq + erlang taken from https://www.rabbitmq.com/install-debian.html#apt-quick-start-packagecloud
## Team RabbitMQ's main signing key
RUN curl -1sLf "https://keys.openpgp.org/vks/v1/by-fingerprint/0A9AF2115F4687BD29803A206B73A36E6026DFCA" | sudo gpg --dearmor | sudo tee /usr/share/keyrings/com.rabbitmq.team.gpg > /dev/null
## Launchpad PPA that provides modern Erlang releases
RUN curl -1sLf "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0xf77f1eda57ebb1cc" | sudo gpg --dearmor | sudo tee /usr/share/keyrings/net.launchpad.ppa.rabbitmq.erlang.gpg > /dev/null
## PackageCloud RabbitMQ repository
RUN curl -1sLf "https://packagecloud.io/rabbitmq/rabbitmq-server/gpgkey" | sudo gpg --dearmor | sudo tee /usr/share/keyrings/io.packagecloud.rabbitmq.gpg > /dev/null
## Add apt repositories maintained by Team RabbitMQ
RUN printf "\
deb [signed-by=/usr/share/keyrings/net.launchpad.ppa.rabbitmq.erlang.gpg] http://ppa.launchpad.net/rabbitmq/rabbitmq-erlang/ubuntu focal main \n\
deb-src [signed-by=/usr/share/keyrings/net.launchpad.ppa.rabbitmq.erlang.gpg] http://ppa.launchpad.net/rabbitmq/rabbitmq-erlang/ubuntu focal main \n\
deb [signed-by=/usr/share/keyrings/io.packagecloud.rabbitmq.gpg] https://packagecloud.io/rabbitmq/rabbitmq-server/ubuntu/ focal main \n\
deb-src [signed-by=/usr/share/keyrings/io.packagecloud.rabbitmq.gpg] https://packagecloud.io/rabbitmq/rabbitmq-server/ubuntu/ focal main\n" \
	| sudo tee /etc/apt/sources.list.d/rabbitmq.list
## Update package indices
RUN sudo apt-get update -y
## Install Erlang packages
RUN sudo apt-get install -y erlang-base \
				erlang-asn1 erlang-crypto erlang-eldap erlang-ftp erlang-inets \
				erlang-mnesia erlang-os-mon erlang-parsetools erlang-public-key \
				erlang-runtime-tools erlang-snmp erlang-ssl \
				erlang-syntax-tools erlang-tftp erlang-tools erlang-xmerl
# hackery to disable autostart at installation https://askubuntu.com/questions/74061/install-packages-without-starting-background-processes-and-services
RUN mkdir /tmp/fake && ln -s /bin/true/ /tmp/fake/initctl && \
				ln -s /bin/true /tmp/fake/invoke-rc.d && \
				ln -s /bin/true /tmp/fake/restart && \
				ln -s /bin/true /tmp/fake/start && \
				ln -s /bin/true /tmp/fake/stop && \
				ln -s /bin/true /tmp/fake/start-stop-daemon && \
				ln -s /bin/true /tmp/fake/service && \
				ln -s /bin/true /tmp/fake/deb-systemd-helper
RUN sudo PATH=/tmp/fake:$PATH apt-get install -y rabbitmq-server

# add expose ports
EXPOSE 8080-8100
EXPOSE 15672
EXPOSE 5672

# build and install additional dev packages
COPY src/ /app

RUN cd /app/amqp-cpp && mkdir build && \
    cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install

RUN cd /app/clickhouse-cpp && mkdir build && \
    cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install

RUN apt update && apt install libssl-dev openssl

RUN cd /app/grpc && mkdir -p cmake/build && cd cmake/build && cmake ../.. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
	-DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DgRPC_SSL_PROVIDER=package \
	-DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF \
	&& make -j $(nproc) && make install

# remove sources
RUN rm -rf /app/amqp-cpp && rm -rf /app/clickhouse-cpp && rm -rf /app/grpc

# install pip requirements
RUN pip3 install -r /app/requirements.txt

# fix for work porto layers
RUN mkdir -p /place/berkanavt/ && apt install fuse dupload libuv1 libuv1-dev

# add paths
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin:/usr/lib/postgresql/14/bin:${PATH}
