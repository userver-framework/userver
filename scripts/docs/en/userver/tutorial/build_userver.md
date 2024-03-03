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


Alternatively you could use a docker container with all the build dependencies
installed from @ref scripts/docs/en/userver/tutorial/build.md

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
