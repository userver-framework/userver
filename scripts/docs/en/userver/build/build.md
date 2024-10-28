# Configure, Build and Install

@anchor quick_start_for_beginners
## Quick start for beginners

If you are new to userver it is a good idea to start with using the `service template` git repository to design your first userver-based service:

* https://github.com/userver-framework/service_template

1\. Clone `service template` and userver repositories.


```shell
git clone --depth 1 https://github.com/userver-framework/service_template.git && \
git clone --depth 1 https://github.com/userver-framework/userver.git service_template/third_party/userver && \
cd service_template
```

More information about `hello service` can be found here: @ref scripts/docs/en/userver/tutorial/hello_service.md

2\. Install userver dependencies.

There is an option to use a @ref docker_with_ubuntu_22_04 "docker container".

Alternatively you can install @ref scripts/docs/en/userver/build/dependencies.md "build dependencies" for userver.

For example to install @ref ubuntu_22_04 build dependencies:

```shell
sudo apt update && \
sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-22.04.md | tr '\n' ' ')
```

3\. Build and start your service.

```shell
make build-release && \
make service-start-release
```

During the build, you can make a coffee break until approximately the following output appears:

```shell
====================================================================================================
Started service at http://localhost:8080/, configured monitor URL is http://localhost:-1/
====================================================================================================

PASSED
[service-runner] Service started, use C-c to terminate
INFO     <userver> Server is started
...
DEBUG    <userver> Accepted connection #2/32768
DEBUG    <userver> Incoming connection from ::ffff:127.0.0.1, fd 43

```

4\. Try to send a request.

```shell
curl http://127.0.0.1:8080/hello?name=userver
```

The answer should be something like this:

```shell
> curl http://127.0.0.1:8080/hello?name=userver
Hello, userver!
```

5\. Now you are ready for fast and comfortable creation of C++ microservices, services and utilities!

@anchor service_templates
## Service templates for userver based services

There are prepared and ready to use service templates at the github:

* https://github.com/userver-framework/service_template
* https://github.com/userver-framework/pg_service_template
* https://github.com/userver-framework/pg_grpc_service_template

Just use the template to make your own service:

1. Press the green "Use this template" button at the top of the github template page
2. Clone the service `git clone your-service-repo && cd your-service-repo`
3. Give a proper name to your service and replace all the occurrences of "*service_template" string with that name.
4. Feel free to tweak, adjust or fully rewrite the source code of your service.

For local development of your service either

* use the docker build and tests run via `make docker-test`;
* or install the build dependencies on your local system and
  adjust the `Makefile.local` file to provide \b platform-specific \b CMake
  options to the template:

The service templates allow to kickstart the development of your production-ready service,
but there can't be a repo for each and every combination of userver libraries.
To use additional userver libraries, e.g. `userver::grpc`, add to the root `CMakeLists.txt`:

```cmake
set(USERVER_FEATURE_GRPC ON CACHE BOOL "" FORCE)
# ...
add_subdirectory(third_party/userver)
# ...
target_link_libraries(${PROJECT_NAME} userver::grpc)
```

@see @ref userver_libraries

See @ref tutorial_services for minimal usage examples of various userver libraries.


## Downloading packages using CPM

userver uses [CPM](https://github.com/cpm-cmake/CPM.cmake) for downloading missing packages.

By default, we first try to find the corresponding system package.
With `USERVER_FORCE_DOWNLOAD_PACKAGES=ON`, no such attempt is made, and we skip straight to `CPMAddPackage`.
This can be useful to guarantee that the build uses the latest (known to userver) versions of third-party packages.

You can control the usage of CPM with `USERVER_DOWNLOAD_*` options. See @ref cmake_options.
For example, `USERVER_DOWNLOAD_PACKAGES=OFF` is useful for CI and other controlled environments
to make sure that no download attempts are made, which ensures repeatability and the best CI build speed.

For details on the download options, refer to [CPM](https://github.com/cpm-cmake/CPM.cmake) documentation.
Some advice:

- `CPM_SOURCE_CACHE` helps to avoid re-downloads with multiple userver build modes or multiple CPM-using projects;
- `CPM_USE_NAMED_CACHE_DIRECTORIES` (which userver enables by default) avoids junk library names shown in IDEs.

@anchor userver_cpm
### Downloading userver using CPM

userver itself can be downloaded and built using CPM.

```cmake
CPMAddPackage(
    NAME userver
    VERSION (userver release version or git commit hash)
    GIT_TAG (userver release version or git commit hash)
    GITHUB_REPOSITORY userver-framework/userver
    OPTIONS
    "USERVER_FEATURE_GRPC ON"
)

target_link_libraries(${PROJECT_NAME} userver::grpc)
```

First, install build dependencies. There are options:

* install @ref scripts/docs/en/userver/build/dependencies.md "build dependencies"
* or use base image of @ref docker_with_ubuntu_22_04

Make sure to enable the CMake options to build userver libraries you need,
then link to those libraries.

@see @ref cmake_options
@see @ref userver_libraries


@anchor userver_install
## Install

You can install userver globally and then use it from anywhere with `find_package`.
Make sure to use the same build mode as for your service, otherwise subtle linkage issues will arise.

### Install with cmake --install

To install userver build it with `USERVER_INSTALL=ON` flags in `Debug` and `Release` modes:
```cmake
cmake -S./ -B./build_debug \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUSERVER_INSTALL=ON \
    -DUSERVER_SANITIZE="ub addr" \
    -GNinja
cmake -S./ -B./build_release \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSERVER_INSTALL=ON \
    -GNinja
cmake --build build_debug/
cmake --build build_release/
cmake --install build_debug/
cmake --install build_release/
```

### Build and install Debian package

To build `libuserver-all-dev.deb` package run the following shell command:

```shell
docker run --rm -it --network ip6net -v $(pwd):/home/user -w /home/user/userver \
   --entrypoint bash ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest ./scripts/docker/run_as_user.sh \
   ./scripts/build_and_install_all.sh
```

And install the package with the following:

```shell
sudo dpkg -i ./libuserver-all-dev*.deb
```

### Use userver in your projects

Next, write

```cmake
find_package(userver REQUIRED COMPONENTS core postgresql grpc redis clickhouse mysql rabbitmq mongo)
```

in your `CMakeLists.txt`. Choose only the necessary components.

@see @ref userver_libraries

Finally, link your source with userver component.

For instance:

```cmake
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::postgresql)
```

Done! You can use installed userver.

### Additional information:

Link `mariadbclient` additionally for mysql component:

```cmake
target_link_libraries(${PROJECT_NAME}-mysql_objs PUBLIC userver::mysql mariadbclient)
```


@anchor docker_with_ubuntu_22_04
## Docker with Ubuntu 22.04

The Docker images provide a container with all the build dependencies preinstalled and 
with a proper setup of PPAs with databases, compilers and tools:

Image reference                                              | Contains                               |
------------------------------------------------------------ | -------------------------------------- |
`ghcr.io/userver-framework/ubuntu-22.04-userver-pg:latest`   | PostgreSQL and userver preinstalled    |
`ghcr.io/userver-framework/ubuntu-22.04-userver:latest`      | userver preinstalled                   |
`ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest` | only userver dependencies preinstalled |

To run it just use a command like

```shell
docker run --rm -it --network ip6net --entrypoint bash ghcr.io/userver-framework/ubuntu-22.04-userver-pg:latest
```

After that, install the databases and compiler you are planning to use via
`apt install` and start developing.

To install userver in `ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest` you should run the following command

```shell
sudo ./scripts/build_and_install_all.sh
```

Alternatively see @ref userver_install

@note The above image is build from `scripts/docker/ubuntu-22.04-pg.dockerfile`,
   `scripts/docker/ubuntu-22.04.dockerfile`
   and `scripts/docker/base-ubuntu-22.04.dockerfile` respectively.
   See `scripts/docker/` directory and @ref scripts/docker/Readme.md for more
   inspiration on building your own custom docker containers.


@anchor userver_conan
## Conan

@note conan must have version >= 1.51, but < 2.0

Thanks to Open-Source community we have Conan support.

You must run the following in the userver directory:

```shell
conan profile new --detect default && conan profile update settings.compiler.libcxx=libstdc++11 default
conan create . --build=missing -pr:b=default -tf conan/test_package/
```

Make sure to pass flags corresponding to the desired userver libraries, e.g. `--with_grpc=1`

Now you can use userver as conan package and build it in your services:

```cmake
target_link_libraries(${PROJECT_NAME} PUBLIC userver::grpc)
```

@see @ref userver_libraries


## Yandex Cloud with Ubuntu 22.04

The userver framework is
[available at Yandex Cloud Marketplace](https://yandex.cloud/en/marketplace/products/yc/userver).

To create a VM with preinstalled userver just click the "Create VM" button and
pay for the Cloud hardware usage.

After that the VM is ready to use. SSH to it and use
`find_package(userver REQUIRED)` in the `CMakeLists.txt` to use the preinstalled
userver framework.

You can start with @ref service_templates "service template".

If there a need to update the userver in the VM do the following:

```bash
sudo apt remove libuserver-*

cd /app/userver
sudo git checkout develop
sudo git pull
sudo ./scripts/build_and_install_all.sh
```

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/faq.md | @ref scripts/docs/en/userver/build/dependencies.md ⇨
@htmlonly </div> @endhtmlonly
