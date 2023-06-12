# userver: Universal Functionality

This folder takes `../shared` and creates a separate CMake target out of it.

`userver-universal` target can be used by projects that want to reuse some
userver utils without including the asynchronous engine.

It should NOT be used alongside `../core`.
