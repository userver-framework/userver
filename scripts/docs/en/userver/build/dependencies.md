# Build dependencies

@note In case you struggle with setting up userver dependencies there are other options:

* @ref docker_with_ubuntu_22_04 "Docker"
* @ref userver_conan "Conan"


## Platform-specific build dependencies

Paths to dependencies start from the @ref service_templates "directory of your service".

@anchor ubuntu_24_04
### Ubuntu 24.04 (Noble Numbat)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-24.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-24.04.md"

Dependencies could be installed via:

```bash
sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-24.04.md | tr '\n' ' ')
```

@anchor ubuntu_22_04
### Ubuntu 22.04 (Jammy Jellyfish)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-22.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-22.04.md"

Dependencies could be installed via:

```bash
sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-22.04.md | tr '\n' ' ')
```

### Ubuntu 21.10 (Impish Indri)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-21.10.md "third_party/userver/scripts/docs/en/deps/ubuntu-21.10.md"

Dependencies could be installed via:

```bash
sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-21.10.md | tr '\n' ' ')
```


### Ubuntu 20.04 (Focal Fossa)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-20.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-20.04.md"

Dependencies could be installed via:

```bash
sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-20.04.md | tr '\n' ' ')
```

\b Recommended \b Makefile.local:

```cmake
CMAKE_COMMON_FLAGS += \
    -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 \
    -DUSERVER_FEATURE_REDIS_HI_MALLOC=1
```


### Ubuntu 18.04 (Bionic Beaver)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-18.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-18.04.md"

Dependencies could be installed via:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-18.04.md | tr '\n' ' ')
  ```

\b Recommended \b Makefile.local:

```cmake
CMAKE_COMMON_FLAGS += \
    -DCMAKE_CXX_COMPILER=g++-8 \
    -DCMAKE_C_COMPILER=gcc-8 \
    -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 \
    -DUSERVER_FEATURE_CRYPTOPP_BASE64_URL=0 \
    -DUSERVER_USE_LD=gold
```


### Debian 11

\b Dependencies: @ref scripts/docs/en/deps/debian-11.md "third_party/userver/scripts/docs/en/deps/debian-11.md"


### Debian 11 32-bit

\b Dependencies: @ref scripts/docs/en/deps/debian-11.md "third_party/userver/scripts/docs/en/deps/debian-11.md" (same as above)

\b Recommended \b Makefile.local:

```cmake
CMAKE_COMMON_FLAGS += \
    -DCMAKE_C_FLAGS='-D_FILE_OFFSET_BITS=64' \
    -DCMAKE_CXX_FLAGS='-D_FILE_OFFSET_BITS=64'
```

### Fedora 35

\b Dependencies: @ref scripts/docs/en/deps/fedora-36.md "third_party/userver/scripts/docs/en/deps/fedora-35.md"

Fedora dependencies could be installed via:

```bash
sudo dnf install -y $(cat third_party/userver/scripts/docs/en/deps/fedora-35.md | tr '\n' ' ')
```

\b Recommended \b Makefile.local:

```cmake
CMAKE_COMMON_FLAGS += \
    -DUSERVER_FEATURE_STACKTRACE=0 \
    -DUSERVER_FEATURE_PATCH_LIBPQ=0
```

### Fedora 36

\b Dependencies: @ref scripts/docs/en/deps/fedora-36.md "third_party/userver/scripts/docs/en/deps/fedora-36.md"

Fedora dependencies could be installed via:

```bash
sudo dnf install -y $(cat third_party/userver/scripts/docs/en/deps/fedora-36.md | tr '\n' ' ')
```

\b Recommended \b Makefile.local:

```cmake
CMAKE_COMMON_FLAGS += \
    -DUSERVER_FEATURE_STACKTRACE=0 \
    -DUSERVER_FEATURE_PATCH_LIBPQ=0
```

### Gentoo

\b Dependencies: @ref scripts/docs/en/deps/gentoo.md "third_party/userver/scripts/docs/en/deps/gentoo.md"

Dependencies could be installed via:
  
```bash
sudo emerge --ask --update --oneshot $(cat scripts/docs/en/deps/gentoo.md | tr '\n' ' ')
```

\b Recommended \b Makefile.local:

```cmake
CMAKE_COMMON_FLAGS += \
    -DUSERVER_CHECK_PACKAGE_VERSIONS=0
```


### Arch, Manjaro

\b Dependencies: @ref scripts/docs/en/deps/arch.md "third_party/userver/scripts/docs/en/deps/arch.md"

Using an AUR helper (pikaur in this example) the dependencies could be installed as:

```bash
pikaur -S $(cat third_party/userver/scripts/docs/en/deps/arch.md | sed 's/^makepkg|//g' | tr '\n' ' ')
```

Without AUR:

```bash
sudo pacman -S $(cat third_party/userver/scripts/docs/en/deps/arch.md | grep -v -- 'makepkg|' | tr '\n' ' ')
cat third_party/userver/scripts/docs/en/deps/arch.md | grep -oP '^makepkg\|\K.*' | while read ;\
  do \
    DIR=$(mktemp -d) ;\
    git clone https://aur.archlinux.org/$REPLY.git $DIR ;\
    pushd $DIR ;\
    yes|makepkg -si ;\
    popd ;\
    rm -rf $DIR ;\
  done
```

\b Recommended \b Makefile.local:
  
```cmake
CMAKE_COMMON_FLAGS += \
    -DUSERVER_FEATURE_PATCH_LIBPQ=0
```


### MacOS

\b Dependencies: @ref scripts/docs/en/deps/macos.md "third_party/userver/scripts/docs/en/deps/macos.md".
At least MacOS 10.15 required with
[Xcode](https://apps.apple.com/us/app/xcode/id497799835) and
[Homebrew](https://brew.sh/).

Dependencies could be installed via:

```bash
brew install $(cat third_party/userver/scripts/docs/en/deps/macos.md | tr '\n' ' ')
```

\b Recommended \b Makefile.local:
  
```cmake
CMAKE_COMMON_FLAGS += \
    -DUSERVER_CHECK_PACKAGE_VERSIONS=0 \
    -DUSERVER_FEATURE_REDIS_HI_MALLOC=1 \
    -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0
```

After that the `make test` would build and run the service tests.

@warning MacOS is recommended only for development as it may have performance issues in some cases.


### Windows (WSL2)

Windows is not supported, there is no plan to support Windows in the near future.

First, install [WSL2](https://learn.microsoft.com/ru-ru/windows/wsl/) with Ubuntu 24.04. Next, follow @ref ubuntu_24_04 "Ubunty 24.04" instructions.

### Other POSIX based platforms

🐙 **userver** works well on modern POSIX platforms. Follow the cmake hints for the installation of required packets and experiment with the CMake options.

Feel free to provide a PR with instructions for your favorite platform at https://github.com/userver-framework/userver.

If there's a strong need to build \b only the userver and run its tests, then see
@ref scripts/docs/en/userver/build/userver.md

## PostgreSQL versions
If CMake option USERVER_FEATURE_PATCH_LIBPQ is on, then the same developer version of libpq, libpgport and libpgcommon libraries
should be available on the system. If there are multiple versions of those libraries use `USERVER_PG_*` @ref cmake_options "CMake options"
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
