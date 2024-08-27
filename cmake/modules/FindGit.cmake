_userver_module_begin(
    NAME Git
)

_userver_module_find_program(
    NAMES git eg
    PATH_SUFFIXES Git/cmd Git/bin
)

_userver_module_end()
