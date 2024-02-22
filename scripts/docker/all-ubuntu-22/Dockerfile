# main system for docker
FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

# Set UTC timezone
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get install -y \
    redis-server \
    mongodb-org \
    mongodb-org-database \
    mongodb-org-server \
    mongodb-org-shell \
    mongodb-org-mongos \
    mongodb-org-tools \
    postgresql-common \
    clickhouse-server \
    clickhouse-client \
    mariadb-server

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

# set up ramdisk symlink for working tmpfs directory in tests
RUN mkdir -p /ramdrive
RUN mkdir -p /mnt
RUN ln -s /ramdrive /mnt/ramdisk

# add paths
ENV PATH /usr/lib/postgresql/14/bin:${PATH}
