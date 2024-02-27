## Build instructions for userver

Usually there's no need to build userver itself. Refer to
@ref scripts/docs/en/userver/tutorial/build.md
section to make a service from a template and build it.

If there's a strong need to build the userver and run its tests, then:

1. Download and extract the latest release from https://github.com/userver-framework/userver
  ```
  bash
  git clone git@github.com:userver-framework/userver.git
  cd userver
  ```
2. Install the build dependencies from @ref scripts/docs/en/userver/tutorial/build.md
3. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DCMAKE_BUILD_TYPE=Release ..  # Adjust with flags from "Configure and Build" section
  cmake --build . -j$(nproc)
  ```
4. Run tests via `ulimit -n 4096 && ctest -V`


@anchor DOCKER_BUILD
### Docker

@note Currently, only x86_64 and x86 architectures support ClickHouse and
MongoDB drivers
as the native libraries for those databases do not support other architectures.
Those drivers are disabled on other architectures via CMake options.

Docker images in userver provide the following functionality:
- build and start all userver tests:
```
bash
docker-compose run --rm userver-tests
```
- build `hello_service` sample:
```
bash
docker-compose run --rm userver-service-sample
```
or
```
bash
SERVICE_NAME=hello_service docker-compose run --rm userver-service-sample
```
- execute commands in userver development environment:
```
bash
docker-compose run --rm userver-ubuntu bash
```


Each step of the `userver-tests` could be executed separately:

Start CMake:
```
docker-compose run --rm userver-ubuntu bash -c 'cmake $CMAKE_OPTS -B./build -S./'
```
Build userver:
```
docker-compose run --rm userver-ubuntu bash -c 'cd /userver/build && make -j $(nproc)'
```
Run all test:
```
docker-compose run --rm userver-ubuntu bash -c 'cd /userver/build && ulimit -n 4096 && ctest -V'
```

Using make, you can build the service easier

Start cmake:
```
make docker-cmake-debug
```
Build userver:
```
make docker-build-debug
```
Run tests:
```
make docker-test-debug
```
You can replace the debug with a release


To run tests and make sure that the framework works fine use the following command:
```
bash
cd build_release && ulimit -n 4096 && ctest -V
```

If you need to edit or make your own docker image with custom configuration, read about
it at @ref scripts/docker/Readme.md.


@anchor userver_conan
### Conan

@note conan must have version >= 1.51, but < 2.0

Thanks to Open-Source community we have Conan support.

You must run the following in the userver directory:
```
conan profile new --detect default && conan profile update settings.compiler.libcxx=libstdc++11 default
conan create . --build=missing -pr:b=default -tf conan/test_package/
```

Make sure to pass flags corresponding to the desired userver libraries, e.g. `--with_grpc=1`

Now you can use userver as conan package and build it in your services:

```cmake
target_link_libraries(${PROJECT_NAME} PUBLIC userver::grpc)
```

@see @ref userver_libraries


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/driver_guide.md | @ref scripts/docs/en/userver/publications.md ⇨
@htmlonly </div> @endhtmlonly
