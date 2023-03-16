# main system for docker
FROM ubuntu:22.04

ARG TARGETARCH
ARG TARGETVARIANT
RUN echo "*********** ${TARGETARCH}/${TARGETVARIANT} **********"

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
	libjemalloc-dev \
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
	liblz4-dev \
	postgresql-common \
	locales

RUN if [[ $TARGETARCH == "amd64" ]]; then apt install -y \
	libmongoc-dev libbson-dev else \
	echo "Skip install mongoc and bson packages: not support '${TARGETARCH}/${TARGETVARIANT}'"; fi

RUN apt clean all


# add clickhouse repositories
RUN if [[ $TARGETARCH == "amd64"]]; then \
	apt get install -y apt-transport-https ca-certificates dirmngr \
	&& apt key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 8919F6BD2B48D754 \
	&& echo "deb https://packages.clickhouse.com/deb stable main" | sudo tee \
    /etc/apt/sources.list.d/clickhouse.list; \
	else echo "Skip add clickhouse repo: not supports"; fi

# add mongodb repositories
RUN if [[ $TARGETARCH == "amd64"]]; then \
	wget -qO - https://www.mongodb.org/static/pgp/server-5.0.asc | sudo apt-key add - \
	&& echo "deb [ arch=amd64 ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/6.0 multiverse" |\
	sudo tee /etc/apt/sources.list.d/mongodb-org-6.0.list \
	&& echo "deb http://security.ubuntu.com/ubuntu jammy-security main" | sudo tee /etc/apt/sources.list.d/jammy-security.list; \
	else echo "Skip add mongo repo: arch not supports"; fi

RUN apt update && \
    apt -o Dpkg::Options::="--force-confold" upgrade -q -y --force-yes && \
    apt -o Dpkg::Options::="--force-confold" dist-upgrade -q -y --force-yes

RUN if [[ $TARGETARCH == "amd64"]]; then \
	apt install -y --allow-unauthenticated \
    mongodb-org \
	mongodb-org-database \
	mongodb-org-server \
	mongodb-org-shell \
	mongodb-org-mongos \
	mongodb-org-tools \
	clickhouse-server \
	clickhouse-client; else echo "skip install mongo and clickhouse"; fi


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
## Cloudsmith: modern Erlang repository
RUN curl -1sLf https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-erlang/gpg.E495BB49CC4BBE5B.key | sudo gpg --dearmor | sudo tee /usr/share/keyrings/io.cloudsmith.rabbitmq.E495BB49CC4BBE5B.gpg > /dev/null
## Cloudsmith: RabbitMQ repository
#RUN curl -1sLf https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-server/gpg.9F4587F226208342.key | sudo gpg --dearmor | sudo tee /usr/share/keyrings/io.cloudsmith.rabbitmq.9F4587F226208342.gpg > /dev/null

## Add apt repositories maintained by Team RabbitMQ
RUN printf "deb [signed-by=/usr/share/keyrings/io.cloudsmith.rabbitmq.E495BB49CC4BBE5B.gpg] https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-erlang/deb/ubuntu jammy main\n\
deb-src [signed-by=/usr/share/keyrings/io.cloudsmith.rabbitmq.E495BB49CC4BBE5B.gpg] https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-erlang/deb/ubuntu jammy main\n\
#deb [signed-by=/usr/share/keyrings/io.cloudsmith.rabbitmq.9F4587F226208342.gpg] https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-server/deb/ubuntu jammy main\n\
#deb-src [signed-by=/usr/share/keyrings/io.cloudsmith.rabbitmq.9F4587F226208342.gpg] https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-server/deb/ubuntu jammy main\n \
" \
 | tee /etc/apt/sources.list.d/rabbitmq.list

## Update package indices
RUN apt update -y

## Install Erlang packages
RUN apt install -y erlang-base \
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
RUN sudo PATH=/tmp/fake:$PATH apt-get install -y --allow-downgrades rabbitmq-server --fix-missing

# add expose ports
EXPOSE 8080-8100
EXPOSE 15672
EXPOSE 5672

# build and install additional dev packages
COPY src/ /app

RUN cd /app/amqp-cpp && mkdir build && \
    cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install

RUN if [[ $TARGETARCH == "amd64"]]; then \
	cd /app/clickhouse-cpp && mkdir build && \
    cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install; \
	else echo "Skip make clickhouse"; fi

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
