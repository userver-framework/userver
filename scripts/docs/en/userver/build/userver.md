# Build instructions for userver's own samples, tests and benchmarks

@warning if you are new to userver please start with @ref quick_start_for_beginners "quick start for beginners". This section is for opensourse userver development ONLY.

Usually there's no need to build userver itself. Refer to @ref scripts/docs/en/userver/build/build.md "Configure, Build and Install"
section to make a service from a template and build it. If there's a strong need to build the userver and run its tests, then you have two options:

* @ref local_build_userver "Local build"
* @ref devcontainers_userver "Dev Containers"

@anchor local_build_userver
## Local build

1. Download and extract the latest release from https://github.com/userver-framework/userver

   ```bash
   git clone git@github.com:userver-framework/userver.git
   cd userver
   ```

2. Install the @ref scripts/docs/en/userver/build/dependencies.md "build dependencies"

   Alternatively you could use a docker container with all the building dependencies
   installed from @ref docker_with_ubuntu_22_04 "Docker".

3. Build userver:

   ```bash
   mkdir build_debug
   cd build_debug
   # Adjust with flags from "Platform-specific build dependencies" section
   cmake -S .. \
       -DCMAKE_C_COMPILER=clang-18 \
       -DCMAKE_CXX_COMPILER=clang++-18 \
       -DCMAKE_BUILD_TYPE=Debug \
       -DCPM_SOURCE_CACHE=~/cpm \
       -DUSERVER_BUILD_ALL_COMPONENTS=1 \
       -DUSERVER_BUILD_SAMPLES=1 \
       -DUSERVER_BUILD_TESTS=1
   cmake --build . -j$(nproc)
   ```

   @see @ref cmake_options

4. Run tests via `ulimit -n 4096 && ctest -V`

If you need to edit or make your own docker image with custom configuration, read about
it at `scripts/docker/Readme.md`.

@anchor devcontainers_userver
## Dev Containers

Dev Containers is the easiest way to build userver itself with all dependencies preinstalled using VSCode or CLion.

1. Download and extract the latest release from https://github.com/userver-framework/userver.

   ```bash
   git clone git@github.com:userver-framework/userver.git
   cd userver
   ```

2. Install Docker

   * [Linux](https://docs.docker.com/engine/install/);
   * [macOS](https://docs.docker.com/desktop/setup/install/mac-install/).

3. Install IDE extension

   * [VSCode](https://code.visualstudio.com/docs/devcontainers/containers): `ms-vscode-remote.remote-containers`;
   * [CLion](https://www.jetbrains.com/help/clion/connect-to-devcontainer.html): "Dev Containers" (**note:** beta).

4. Open the userver project

   * If CMake asks to configure, deny;
   * For CLion, please use JetBrains Gateway to open the project, otherwise CLion gets confused.

5. Agree to reopen the project in a Dev Container.

6. The Docker container for development will automatically be downloaded (~1.5GB, may take a while), unpacked and run
   using the config from `.devcontainer` directory.

7. CMake will prompt you to select a Kit. Run Scan, then press `Ctrl + Shift + P` and type "CMake: Select a kit". Choose clang-18. After that, project configuration will begin.

8. When project configuration is complete, you can open any source file. clangd will start indexing the entire project in the background, and after indexing is complete, you will have code navigation, find references, and other features available.

9. To build userver, simply click the Build button in the IDE.

10. To run the tests, execute in the build directory: `ulimit -n 4096 && ctest -V`.

11. (Optional)
    [Share Git credentials](https://code.visualstudio.com/remote/advancedcontainers/sharing-git-credentials)
    with the container to perform VCS operations from the IDE.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/build/options.md | @ref scripts/docs/en/userver/tutorial/hello_service.md ⇨
@htmlonly </div> @endhtmlonly
