FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

RUN apt-get update && apt-get install -y sudo curl gnupg lsb-release libclang-dev clangd

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"
RUN rustup default stable

RUN apt-get update && apt-get install -y libuv1-dev

RUN git clone https://github.com/scylladb/cpp-rs-driver.git /opt/cpp-rs-driver \
    && cd /opt/cpp-rs-driver \
    && mkdir build && cd build \
    && cmake -DCASS_BUILD_STATIC=OFF -DCASS_BUILD_INTEGRATION_TESTS=OFF -DCMAKE_BUILD_TYPE=Release .. \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && cd / && rm -rf /opt/cpp-rs-driver


EXPOSE 8080-8100
