# CMake

## Version

CMake minimum supported version is 3.14.

## Generic rules

- prefer N declarative configurations + 1 config processor to N imperative scenarios (config processor -> cmake/, configuration -> the module)
- do not hide errors and typos, hard error on misconfiguration

## Modules

- prefer unified configuration, try to avoid custom single-module settings
- module specific settings must be located in module directories, not in `cmake/*`

## Functions and macros

- prefer functions, avoid macros if possible
- register functions/macros/args in `.cmake-format.py`
- use doxygen-ish way of function/macro documentation with `@arg` (positionals), `@param`/`@multiparam` (1/*-param args), `@option` (0-param args):

    ```cmake
    # Generates ${TARGET} cmake target for C++ types, parsers, serializers from JSONSchema file(s).
    #
    # @arg TARGET smth
    # @param OUTPUT_DIR - where to put generated .cpp/.hpp/.ipp files, usually ${CMAKE_CURRENT_BINARY_DIR}/smth
    # @multiparam LINK_TARGETS - targets to link (used by x-usrv-cpp-type)
    # @option NO_SAX_PARSE - Do not generate SAX parser and efficient FromJsonString() member factory function
    function(userver_target_generate_chaotic TARGET)
        # ... implementation
    endfunction()
    ```

## Users

`*.cmake`, `*.cmake.in` may be used by:
- the userver itself
- an out-of-tree service that does `find_package(userver ...)`

Make sure `*.cmake*` files still work for both userver and out-of-tree services.

## Documentation

Keep the documentation up-to-date:
* scripts/docs/en/userver/build/build.md
* scripts/docs/en/userver/build/dependencies.md
* scripts/docs/en/userver/build/options.md

## Formatting

Format cmake files after the change with `cmake-format -i path/to/cmake/file.cmake`.
