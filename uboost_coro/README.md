# userver: Boost.Coro
Subset of [Boost C++ Libraries](https://www.boost.org) 1.80.0, that includes Coroutine2 and Context.
`CMakeLists.txt` is based on [boost-cmake](https://github.com/Orphis/boost-cmake).

Sanitizers could be enabled via [USERVER_SANITIZE](https://userver.tech/d1/d03/md_en_userver_tutorial_build.html).


## Diff with Upstream
* `boost` directory renamed into `uboost_coro` and moved into `include`.
Includes of `context` and `coroutine2` use the new paths.
* Source codes are moved from `libs/<libname>/src` to `src/<libname>`.
* Removed files for some unsupported architectures and platforms.
