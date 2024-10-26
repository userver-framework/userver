# Build instructions for userver's own samples, tests and benchmarks

@warning if you are new to userver please start with @ref quick_start_for_beginners "quick start for beginners". This section is for opensourse userver development ONLY.

Usually there's no need to build userver itself. Refer to @ref scripts/docs/en/userver/build/build.md "Configure, Build and Install"
section to make a service from a template and build it. If there's a strong need to build the userver and run its tests, then:

1. Download and extract the latest release from https://github.com/userver-framework/userver

```bash
git clone git@github.com:userver-framework/userver.git
cd userver
```

2. Install the @ref scripts/docs/en/userver/build/dependencies.md "build dependencies"

3. Build the userver:

```bash
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..  # Adjust with flags from "Configure and Build" section
cmake --build . -j$(nproc)
```

4. Run tests via `ulimit -n 4096 && ctest -V`


Alternatively you could use a docker container with all the building dependencies
installed from @ref scripts/docs/en/userver/build/ependencies.md "build dependencies"

If you need to edit or make your own docker image with custom configuration, read about
it at @ref scripts/docker/Readme.md.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/build/options.md | @ref scripts/docs/en/userver/tutorial/hello_service.md ⇨
@htmlonly </div> @endhtmlonly
