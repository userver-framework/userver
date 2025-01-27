FROM ubuntu:24.04

COPY scripts/docs/en/deps/ubuntu-24.04.md /userver_tmp/
COPY scripts/postgres/ubuntu-install-postgresql-includes.sh /userver_tmp/

RUN apt update \
  && apt install -y $(cat /userver_tmp/ubuntu-24.04.md) \
  && apt install -y clang-format python3-pip \
  && apt install -y \
    clickhouse-server \
    mariadb-server \
    postgresql-16 \
    rabbitmq-server \
    redis-server \
  && apt install -y locales \
  && apt clean all \
  && /userver_tmp/ubuntu-install-postgresql-includes.sh

RUN \
  # Set UTC timezone \
  TZ=Etc/UTC; \
  ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \
   \
  # Generate locales \
  && locale-gen en_US.UTF-8 \
  && update-locale LC_ALL="en_US.UTF-8" LANG="en_US.UTF-8" LANGUAGE="en_US.UTF-8" 


COPY scripts/clickhouse/ubuntu-install-clickhouse.sh /userver_tmp/
RUN /userver_tmp/ubuntu-install-clickhouse.sh

COPY scripts/rabbitmq/ubuntu_install_rabbitmq_dev.sh /userver_tmp/
RUN /userver_tmp/ubuntu_install_rabbitmq_dev.sh 
