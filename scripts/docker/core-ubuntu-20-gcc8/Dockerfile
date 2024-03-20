# main system for docker
FROM ubuntu:20.04

# Set UTC timezone
ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

ENV DEBIAN_FRONTEND noninteractive

# install main packages
RUN apt-get update && apt-get install -y --allow-unauthenticated \
    ccache \
    chrpath \
    clang-10 \
    lld-10 \
    clang-12 \
    lld-12 \
    g++-8 \
    g++-9 \
    cmake \
    libboost1.71-dev \
    libboost-program-options1.71-dev \
    libboost-filesystem1.71-dev \
    libboost-locale1.71-dev \
    libboost-regex1.71-dev \
    libboost-iostreams1.71-dev \
    libboost-context1.71-dev \
    libboost-coroutine1.71-dev \
    libboost-stacktrace1.71-dev \
    libev-dev \
    zlib1g-dev \
    libcurl4-openssl-dev \
    curl \
    libyaml-cpp-dev \
    libssl-dev \
    libcctz-dev \
    libnghttp2-dev \
    libjemalloc-dev \
    libldap2-dev \
    libkrb5-dev \
    python3-dev \
    python3-jinja2 \
    python3-pip \
    python3-voluptuous \
    python3-yaml \
    python3-venv \
    libbenchmark-dev \
    libgmock-dev \
    libgtest-dev \
    git \
    sudo \
    gnupg \
    gnupg2 \
    wget \
    dirmngr \
    libcrypto++-dev \
    liblz4-dev \
    locales \
    ninja-build

RUN apt-get clean all

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

# add expose ports
EXPOSE 8080-8100

# add requirements
COPY src/ /app

RUN cd /app/fmt && mkdir build && \ 
    cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DFMT_DOC=OFF -DFMT_TEST=OFF -DFMT_INSTALL=ON -DFMT_MASTER_PROJECT=ON \ 
    -DCMAKE_CXX_VISIBILITY_PRESET=hidden -DCMAKE_VISIBILITY_INLINES_HIDDEN=ON -DFMT_PEDANTIC=ON -DFMT_WERROR=ON \
    -DCMAKE_INSTALL_PREFIX=/usr ..\
    && make -j $(nproc) && make install

RUN cd /app/c-ares && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. \
    && make -j $(nproc) && make install

RUN rm -rf /app/fmt /app/c-ares

# install pip requirements
RUN pip3 install -r /app/requirements.txt

# fix for work porto layers
RUN mkdir -p /place/berkanavt/ && apt install fuse dupload libuv1 libuv1-dev

RUN apt clean all

# add paths
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin:${PATH}
