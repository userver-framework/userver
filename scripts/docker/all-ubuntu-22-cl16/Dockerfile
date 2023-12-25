# main system for docker
FROM ghcr.io/userver-framework/ubuntu-userver-core-cl16:v1

# Set UTC timezone
ENV DEBIAN_FRONTEND noninteractive

# install main packages
RUN apt-get update && apt-get install -y --allow-unauthenticated \
    build-essential \
    binutils-dev \
    libmongoc-dev \
    libbson-dev \
    libpq-dev \
    postgresql-server-dev-all \
    libhiredis-dev \
    clang-format \
    clang-tidy \
    postgresql \
    python3-protobuf \
    python3-jinja2 \
    python3-virtualenv \
    vim \
    redis-server

RUN apt-get clean all


# add clickhouse repositories
RUN sudo apt-get install -y apt-transport-https ca-certificates dirmngr
RUN sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 8919F6BD2B48D754

RUN echo "deb https://packages.clickhouse.com/deb stable main" | sudo tee \
    /etc/apt/sources.list.d/clickhouse.list

# add mongodb repositories
RUN wget -qO - https://www.mongodb.org/static/pgp/server-6.0.asc | sudo apt-key add -

RUN echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/6.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-6.0.list

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
    clickhouse-server \
    clickhouse-client

# add mariadb repositories
RUN wget https://downloads.mariadb.com/MariaDB/mariadb_repo_setup
RUN chmod +x mariadb_repo_setup
RUN ./mariadb_repo_setup

RUN apt-get install -y libmariadb-dev mariadb-server

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

RUN pip3 install pep8

# build and install additional dev packages
COPY src/ /app

RUN cd /app/amqp-cpp && mkdir build && \
    cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install

RUN cd /app/clickhouse-cpp && mkdir build && \
    cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc) && make install

RUN apt update && apt install libssl-dev openssl

RUN cd /app/grpc && mkdir -p cmake/build && cd cmake/build && cmake ../.. -DCMAKE_C_COMPILER=clang-16 -DCMAKE_CXX_COMPILER=clang++-16 \
    -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DgRPC_SSL_PROVIDER=package \
    -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF \
    && make -j $(nproc) && make install

# remove sources
RUN rm -rf /app/amqp-cpp && rm -rf /app/clickhouse-cpp && rm -rf /app/grpc

# install pip requirements
RUN pip3 install -r /app/requirements.txt

# set up ramdisk symlink for working tmpfs directory in tests
RUN mkdir -p /ramdrive
RUN mkdir -p /mnt
RUN ln -s /ramdrive /mnt/ramdisk

# add paths
ENV PATH /usr/lib/postgresql/14/bin:${PATH}
