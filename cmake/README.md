# userver: CMake Helpers

CMake scripts that are used by the core CMakeLists.txt

It contains several scripts intended for external usage:

* `UserverSetupEnvironment.cmake` defines `userver_setup_environment` function
  that that simplifies userver usage as a submodule of your project:

  ```cmake
  add_subdirectory(third_party/userver)
  userver_setup_environment()
  ```

* `UserverTestsuite.cmake` defines functions for setting up
  @ref scripts/docs/en/userver/functional_testing.md "testsuite tests":

  ```cmake
  userver_testsuite_add_simple()
  ```

* `GrpcTargets.cmake` defines functions for generating userver gRPC
  clients and services

* `AddGoogleTests.cmake` defines functions for creating CTest tests
  from gtest/utest targets

See https://github.com/userver-framework/service_template for an example.
