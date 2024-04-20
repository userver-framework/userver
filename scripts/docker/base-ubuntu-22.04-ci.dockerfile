FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

# Apply the following:
# * fix for porto layers
# * set up ramdisk symlink for working tmpfs directory in tests
# * install a bunch of databases and compilers 
RUN \
  mkdir -p /place/berkanavt/ && \
  mkdir -p /ramdrive && mkdir -p /mnt && ln -s /ramdrive /mnt/ramdisk && \
  \
  PORTO_WORKAROUND="fuse dupload libuv1 libuv1-dev openssh-client"; \
  DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    $PORTO_WORKAROUND \
    clickhouse-server \
    mariadb-server \
    mongodb-org \
    postgresql-14 \
    rabbitmq-server \
    redis-server \
    clang-16 lld-16 llvm-16 libclang-rt-16-dev\
    clang-14 lld-14 llvm \
    g++-11 gcc-11 \
    g++-13 gcc-13 \
    && \
  pip3 install pep8 && \
  apt clean all

EXPOSE 8080-8100
EXPOSE 15672
EXPOSE 5672
