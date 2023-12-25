# main system for docker
FROM ubuntu:22.04

# Set UTC timezone
ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

ENV DEBIAN_FRONTEND noninteractive

# add llvm-16 repository
RUN apt update && apt-get install -y apt-transport-https ca-certificates dirmngr wget
RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 8919F6BD2B48D754
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
RUN printf "\
deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main \n\
deb-src http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main\n" \
    | tee /etc/apt/sources.list.d/llvm.list

# install main packages
RUN apt-get update && apt-get install -y --allow-unauthenticated \
    ccache \
    chrpath \
    cmake \
	clang-16 \
	lldb-16 \
	lld-16 \
    libboost1.74-dev \
    libboost-program-options1.74-dev \
    libboost-filesystem1.74-dev \
    libboost-locale1.74-dev \
    libboost-regex1.74-dev \
    libboost-iostreams1.74-dev \
    libboost-context1.74-dev \
    libboost-coroutine1.74-dev \
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
    libldap2-dev \
    libkrb5-dev \
    python3-dev \
    virtualenv \
    python3-voluptuous \
    python3-yaml \
    libc-ares-dev \
    libbenchmark-dev \
    libgmock-dev \
    libgtest-dev \
    git \
    sudo \
    gnupg \
    gnupg2 \
    dirmngr \
    libcrypto++-dev \
    liblz4-dev \
	locales

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

# install pip requirements
RUN pip3 install -r /app/requirements.txt

# fix for work porto layers
RUN mkdir -p /place/berkanavt/ && apt install fuse dupload libuv1 libuv1-dev

RUN apt clean all

# add paths
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin:${PATH}
