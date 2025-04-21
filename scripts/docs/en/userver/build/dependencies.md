# Build dependencies

@note In case you struggle with setting up userver dependencies,
there are @ref ways_to_get_userver "options to use prebuilt userver".


## Platform-specific build dependencies

Paths to dependencies start from the @ref service_templates "directory of your service".

@anchor ubuntu_24_04
### Ubuntu 24.04 (Noble Numbat)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-24.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-24.04.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/ubuntu-24.04.md" && \
sudo apt install --allow-downgrades -y $(wget -q -O - ${DEPS_FILE})
```

@anchor ubuntu_22_04
### Ubuntu 22.04 (Jammy Jellyfish)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-22.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-22.04.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/ubuntu-22.04.md" && \
sudo apt install --allow-downgrades -y $(wget -q -O - ${DEPS_FILE})
```

### Ubuntu 21.10 (Impish Indri)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-21.10.md "third_party/userver/scripts/docs/en/deps/ubuntu-21.10.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/ubuntu-21.10.md" && \
sudo apt install --allow-downgrades -y $(wget -q -O - ${DEPS_FILE})
```

@anchor debian_12
### Debian 12

\b Dependencies: @ref scripts/docs/en/deps/debian-12.md "third_party/userver/scripts/docs/en/deps/debian-12.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/debian-12.md" && \
sudo apt install --allow-downgrades -y $(wget -q -O - ${DEPS_FILE})
```

@anchor debian_11
### Debian 11

\b Dependencies: @ref scripts/docs/en/deps/debian-11.md "third_party/userver/scripts/docs/en/deps/debian-11.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/debian-11.md" && \
sudo apt install --allow-downgrades -y $(wget -q -O - ${DEPS_FILE})
```


### Debian 11 32-bit

\b Dependencies are the same as above.

Recommended CMake options:

```Makefile
CMAKE_COMMON_FLAGS += \
    -DCMAKE_C_FLAGS='-D_FILE_OFFSET_BITS=64' \
    -DCMAKE_CXX_FLAGS='-D_FILE_OFFSET_BITS=64'
```

### Fedora 35

\b Dependencies: @ref scripts/docs/en/deps/fedora-36.md "third_party/userver/scripts/docs/en/deps/fedora-35.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/fedora-35.md" && \
sudo dnf install -y $(wget -q -O - ${DEPS_FILE})
```

Recommended CMake options:

```
USERVER_FEATURE_STACKTRACE=0
USERVER_FEATURE_PATCH_LIBPQ=0
```

### Fedora 36

\b Dependencies: @ref scripts/docs/en/deps/fedora-36.md "third_party/userver/scripts/docs/en/deps/fedora-36.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/fedora-36.md" && \
sudo dnf install -y $(wget -q -O - ${DEPS_FILE})
```

Recommended CMake options:

```
USERVER_FEATURE_STACKTRACE=0
USERVER_FEATURE_PATCH_LIBPQ=0
```

### Gentoo

\b Dependencies: @ref scripts/docs/en/deps/gentoo.md "third_party/userver/scripts/docs/en/deps/gentoo.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/gentoo.md" && \
sudo emerge --ask --update --oneshot $(wget -q -O - ${DEPS_FILE})
```

Recommended CMake options:

```
USERVER_CHECK_PACKAGE_VERSIONS=0
```


### Alpine

\b Dependencies: @ref scripts/docs/en/deps/alpine.md "third_party/userver/scripts/docs/en/deps/alpine.md"

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/alpine.md" && \
sudo apk add $(wget -q -O - ${DEPS_FILE})
```

Recommended CMake options:

```
USERVER_FEATURE_JEMALLOC=OFF
USERVER_FEATURE_STACKTRACE=OFF
USERVER_FEATURE_KAFKA=OFF
USERVER_DOWNLOAD_PACKAGE_PROTOBUF=ON
USERVER_DISABLE_RSEQ_ACCELERATION=YES
```


### Arch, Manjaro

\b Dependencies: @ref scripts/docs/en/deps/arch.md "third_party/userver/scripts/docs/en/deps/arch.md"

Using an AUR helper (pikaur in this example) the dependencies can be installed as:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/arch.md" && \
pikaur -S $(wget -q -O - ${DEPS_FILE} | sed 's/^makepkg|//g')
```

Without AUR:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/arch.md" && \
sudo pacman -S $(wget -q -O - ${DEPS_FILE} | grep -v -- 'makepkg|')
wget -q -O - ${DEPS_FILE} | grep -oP '^makepkg\|\K.*' | while read ;\
  do \
    DIR=$(mktemp -d) ;\
    git clone https://aur.archlinux.org/$REPLY.git $DIR ;\
    pushd $DIR ;\
    yes|makepkg -si ;\
    popd ;\
    rm -rf $DIR ;\
  done
```

Recommended CMake options:

```
USERVER_FEATURE_PATCH_LIBPQ=0
```


### macOS

\b Dependencies: @ref scripts/docs/en/deps/macos.md "third_party/userver/scripts/docs/en/deps/macos.md".
At least macOS 10.15 required with
[Xcode](https://apps.apple.com/us/app/xcode/id497799835) and
[Homebrew](https://brew.sh/).

Dependencies can be installed via:

```bash
DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/arch.md" && \
brew install $(wget -q -O - ${DEPS_FILE})
```

Some Homebrew packages are keg-only (they are not symlinked to Homebrew prefix path e.g. `/opt/homebrew`),
therefore they can not be found by CMake in configure phase.

It is possible to symlink the libraries using `brew link --force`.

For current macOS build dependencies, userver expects that following packages are symlinked:

```bash
brew link postgresql@16 # postgresql is keg-only (required by PostgreSQL)
brew link --force openldap # keg-only (required by PostgreSQL)
brew link --force zlib # keg-only + need for static linkage (required by Universal)
brew link --force icu4c # keg-only + need for static boost linkage (required by Universal)
brew link --force curl # keg-only (required by Core)
brew link --force cyrus-sasl # keg-only (required by Mongo and Kafka)
```

Recommended CMake options:

```Makefile
USERVER_CHECK_PACKAGE_VERSIONS=0
USERVER_FEATURE_CRYPTOPP_BLAKE2=0
USERVER_FORCE_DOWNLOAD_ABSEIL=1
USERVER_FORCE_DOWNLOAD_RE2=1
USERVER_FORCE_DOWNLOAD_PROTOBUF=1
USERVER_FORCE_DOWNLOAD_GRPC=1
```

After that the `make test` would build and run the service tests.

@warning MacOS is recommended only for development as it may have performance issues in some cases.

@warning If you have installed `abseil` with Homebrew, it may be required to remove it, because it can conflict with CPM installed abseil (`brew remove --ignore-dependencies abseil`)


### Windows (WSL2)

Windows native development is not supported, there is no plan to support Windows native development in the near future. You need to use [WSL2](https://learn.microsoft.com/en-us/windows/wsl/).

First, install [WSL2](https://learn.microsoft.com/en-us/windows/wsl/) with Ubuntu 24.04. Next, follow @ref ubuntu_24_04 "Ubuntu 24.04" instructions.

### Other POSIX based platforms

🐙 **userver** works well on modern POSIX platforms. Follow the cmake hints for the installation of required packets and experiment with the CMake options.

Feel free to provide a PR with instructions for your favorite platform at https://github.com/userver-framework/userver.

If there's a strong need to build \b only the userver and run its tests, then see
@ref scripts/docs/en/userver/build/userver.md

@anchor postgres_deps_versions
## PostgreSQL versions

If CMake option `USERVER_FEATURE_PATCH_LIBPQ` is on, then the same developer version of libpq, libpgport and libpgcommon libraries
should be available on the system. If there are multiple versions of those libraries, use `USERVER_PG_*` @ref cmake_options "CMake options"
to aid the build system in finding the right version.

You could also install any version of the above libraries by explicitly pinning the version. For example in Debian/Ubuntu pinning
to version 14 could be done via the following commands:

```shell
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 7FCC7D46ACCC4CF8
echo "deb https://apt-archive.postgresql.org/pub/repos/apt jammy-pgdg-archive main" | sudo tee /etc/apt/sources.list.d/pgdg.list
sudo apt update

sudo mkdir -p /etc/apt/preferences.d

printf "Package: postgresql-14\nPin: version 14.5*\nPin-Priority: 1001\n" | sudo tee -a /etc/apt/preferences.d/postgresql-14
printf "Package: postgresql-client-14\nPin: version 14.5*\nPin-Priority: 1001\n" | sudo tee -a /etc/apt/preferences.d/postgresql-client-14
sudo apt install --allow-downgrades -y postgresql-14 postgresql-client-14
 
printf "Package: libpq5\nPin: version 14.5*\nPin-Priority: 1001\n" | sudo tee -a /etc/apt/preferences.d/libpq5
printf "Package: libpq-dev\nPin: version 14.5*\nPin-Priority: 1001\n"| sudo tee -a /etc/apt/preferences.d/libpq-dev
sudo apt install --allow-downgrades -y libpq5 libpq-dev
```

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/build/build.md | @ref scripts/docs/en/userver/build/options.md ⇨
@htmlonly </div> @endhtmlonly
