_userver_module_begin(
    NAME
    PythonDev
    DEBIAN_NAMES
    python3-dev
    FORMULA_NAMES
    python3-dev
    RPM_NAMES
    python3-dev
    PACMAN_NAMES
    python
)

# minimum version is bound by testsuite
_userver_module_find_include(
    NAMES
    Python.h
    PATHS
    /usr/include/python3m
    /usr/include/python3.9m
    /usr/include/python3
    /usr/include/python3.9
    /usr/include/python3.10
    /usr/include/python3.11
    /usr/include/python3.12
    /usr/include/python3.13
    /usr/include/python3.14
)

_userver_module_end()
