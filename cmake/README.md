# userver: CMake Helpers

CMake scripts that are used by the core CMakeLists.txt

Most importantly, it contains the `SetupEnvironment.cmake` file that simplifies
userver usage as a submodule of your project:

```
include(third_party/userver/cmake/SetupEnvironment.cmake)
add_subdirectory(third_party/userver)
```

See https://github.com/userver-framework/service_template for an example.
