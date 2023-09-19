# userver: Boost.Coro
Subset of [Boost C++ Libraries](https://www.boost.org) 1.83.0, that includes Coroutine2 and Context.
`CMakeLists.txt` is based on [boost-cmake](https://github.com/Orphis/boost-cmake).

Sanitizers could be enabled via [USERVER_SANITIZE](https://userver.tech/d3/da9/md_en_2userver_2tutorial_2build.html).


## Diff with Upstream
* `boost` directory renamed into `uboost_coro` via
  `find . -type f | xargs sed -i 's|boost/context/|uboost_coro/context/|g'` and
  `find . -type f | xargs sed -i 's|boost/coroutine2/|uboost_coro/coroutine2/|g'`.
  Moved into `include`. Includes of `context` and `coroutine2` use the new paths.
* Source codes are moved from `libs/<libname>/src` to `src/<libname>`.
* Removed files for some unsupported architectures and platforms.
