FROM ubuntu:22.04

COPY scripts/docs/en/deps/ubuntu-22.04.md /userver_tmp/
COPY scripts/docker/setup-base-ubuntu-22.04-env.sh /userver_tmp/

COPY scripts/external_deps/requirements.txt  /userver_tmp/requirements/external-deps.txt
COPY scripts/grpc/requirements-3.txt         /userver_tmp/requirements/grpc-userver.txt
#COPY scripts/chaotic/requirements.txt       /userver_tmp/requirements/chaotic.txt
#COPY testsuite/requirements-ydb.txt         /userver_tmp/requirements/ydb.txt
COPY testsuite/requirements-grpc-3.txt       /userver_tmp/requirements/grpc.txt
COPY testsuite/requirements-mongo.txt        /userver_tmp/requirements/mongo.txt
COPY testsuite/requirements-postgres.txt     /userver_tmp/requirements/postgres.txt
COPY testsuite/requirements-redis.txt        /userver_tmp/requirements/redis.txt
COPY testsuite/requirements-testsuite.txt    /userver_tmp/requirements/testsuite.txt
COPY testsuite/requirements-net.txt          /userver_tmp/requirements/net.txt
COPY testsuite/requirements.txt              /userver_tmp/requirements/testsuite-base.txt

COPY scripts/docker/pip-install.sh           /userver_tmp/

RUN ( \
  cd /userver_tmp \
  && ./setup-base-ubuntu-22.04-env.sh \
  && ./pip-install.sh \
  && mkdir /app && cd /app && git clone --depth 1 -b 1.50.0 https://github.com/googleapis/api-common-protos.git && rm -rf /app/api-common-protos/.git \
  && rm -rf /userver_tmp \
)

# add expose ports
EXPOSE 8080-8100
