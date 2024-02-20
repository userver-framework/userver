apt update
apt install -y apt-transport-https ca-certificates dirmngr wget software-properties-common
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 8919F6BD2B48D754
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
printf "\
deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main \n\
deb-src http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main\n" \
    | tee /etc/apt/sources.list.d/llvm.list

add-apt-repository ppa:ubuntu-toolchain-r/test

# Install additional compilers main packages
DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends --allow-unauthenticated \
    clang-16 \
    lld-16 \
    clang-14 \
    lld-14 \
    g++-11 \
    gcc-11 \
    g++-13 \
    gcc-13

