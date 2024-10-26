## Non-Coroutine Console Tool

## Before you start

Make sure that you can compile and run framework tests as described at
@ref scripts/docs/en/userver/build/build.md.


## Step by step guide

The userver framework allows to use it's non-coroutine parts by using the
`userver-universal` CMake target. It provides usefull utilities like
utils::FastPimpl, utils::TrivialBiMap,
@ref scripts/docs/en/userver/formats.md "JSON and YAML formats", utils::AnyMovable,
cache::LruMap and many other utilities. See  @ref userver_universal for a list
of available functions and classes.


Let's write a simple JSON to YAML converter with the help of `userver-universal`.

### Code

The implementation is quite straightforward. Include the necessary C++ Standard
library and userver headers:

@snippet samples/json2yaml/src/json2yaml.hpp  json2yaml - includes

Write the logic for converting each of the JSON types to YAML type:

@snippet samples/json2yaml/src/json2yaml.hpp  json2yaml - convert hpp
@snippet samples/json2yaml/src/json2yaml.cpp  json2yaml - convert cpp

Finally, read data from `std::cin`, parse it as JSON, convert to YAML and
output it as text:

@snippet samples/json2yaml/main.cpp  json2yaml - main


### Build and Run

To build the sample, execute the following build steps at the
`userver/samples/json2yaml` (if userver is installed into the system) or from
userver root directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-json2yaml
```

After that a tool is compiled and it could be used:
```
bash
$ samples/json2yaml/userver-samples-json2yaml
{"key": {"nested-key": [1, 2.0, "hello!", {"key-again": 42}]}}
^D
key:
  nested-key:
    - 1
    - 2
    - hello!
    - key-again: 42

```


### Testing

The code could be tested using any of the unit-testing frameworks, like
Boost.Test, GTest, and so forth. Here's how to do it with GTest:

* Write a test:
  @snippet samples/json2yaml/unittests/json2yaml_test.cpp  json2yaml - unittest
* Add its run to CMakeLists.txt:
  @snippet samples/json2yaml/CMakeLists.txt  add_unit_test
* Run the test via `ctest -V`

The above code tests the conversion logic, but not the final binary that may
have issues in invoking the above logic. To test the final binary let's use
Python with `pytest`:

* Inform CMake about the test and that it should be started in a python venv
  with `pytest` installed. Pass the path to the CMake built binary to venv:
  @snippet samples/json2yaml/CMakeLists.txt  add_test

* Add a fixture to `conftest.py` to get the path to the CMake built binary:
  @snippet samples/json2yaml/testsuite/conftest.py  pytest

* Write the test:
  @snippet samples/json2yaml/testsuite/test_basic.py  pytest


## Full sources

See the full example at:
* @ref samples/json2yaml/json2yaml.hpp
* @ref samples/json2yaml/json2yaml.cpp
* @ref samples/json2yaml/json2yaml_test.cpp
* @ref samples/json2yaml/CMakeLists.txt
* @ref samples/json2yaml/tests/conftest.py
* @ref samples/json2yaml/tests/test_basic.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/multipart_service.md | @ref scripts/docs/en/userver/component_system.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/json2yaml/json2yaml.hpp
@example samples/json2yaml/json2yaml.cpp
@example samples/json2yaml/json2yaml_test.cpp
@example samples/json2yaml/CMakeLists.txt
@example samples/json2yaml/tests/conftest.py
@example samples/json2yaml/tests/test_basic.py
