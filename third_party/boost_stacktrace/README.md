### Stacktrace
Library for storing and printing backtraces.
Taken from https://github.com/boostorg/stacktrace/commit/7fedfa12654d18a9fa695de258763e93699c4636 (Boost 1.68.0)
libbacktrace_impls.hpp updated from upstream https://github.com/boostorg/stacktrace/blob/4123beb4af6ff4e36769905b87c206da39190847/include/boost/stacktrace/detail/libbacktrace_impls.hpp to avoid big memory consumption and to speed up stack decoding.

[Documentation and examples.](http://boostorg.github.io/stacktrace/index.html)


### Test results
@               | Build         | Tests coverage | More info
----------------|-------------- | -------------- |-----------
Develop branch:  | [![Build Status](https://travis-ci.org/boostorg/stacktrace.svg?branch=develop)](https://travis-ci.org/boostorg/stacktrace) [![Build status](https://ci.appveyor.com/api/projects/status/l3aak4j8k39rx08t/branch/develop?svg=true)](https://ci.appveyor.com/project/apolukhin/stacktrace/branch/develop) | [![Coverage Status](https://coveralls.io/repos/github/boostorg/stacktrace/badge.svg?branch=develop)](https://coveralls.io/github/boostorg/stacktrace?branch=develop) | [details...](http://www.boost.org/development/tests/develop/developer/stacktrace.html)
Master branch:  | [![Build Status](https://travis-ci.org/boostorg/stacktrace.svg?branch=master)](https://travis-ci.org/boostorg/stacktrace) [![Build status](https://ci.appveyor.com/api/projects/status/l3aak4j8k39rx08t/branch/master?svg=true)](https://ci.appveyor.com/project/apolukhin/stacktrace/branch/master) | [![Coverage Status](https://coveralls.io/repos/github/boostorg/stacktrace/badge.svg?branch=master)](https://coveralls.io/github/boostorg/stacktrace?branch=master) | [details...](http://www.boost.org/development/tests/master/developer/stacktrace.html)


### License
Distributed under the [Boost Software License, Version 1.0](http://boost.org/LICENSE_1_0.txt).
