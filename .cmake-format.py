# ----------------------------------
# Options affecting listfile parsing
# ----------------------------------
with section('parse'):
    # Specify structure for custom cmake functions
    # pargs - positional arguments
    # kwargs - keyword arguments
    additional_commands = {
        'userver_module': {
            'pargs': 1,
            'kwargs': {
                'SOURCE_DIR': '*',
                'INSTALL_COMPONENT': '*',
                'LINK_LIBRARIES': '*',
                'LINK_LIBRARIES_PRIVATE': '*',
                'UTEST_SOURCES': '*',
                'UTEST_DIRS': '*',
                'UTEST_LINK_LIBRARIES': '*',
                'INCLUDE_DIRS': '*',
                'DBTEST_SOURCES': '*',
                'DBTEST_DATABASES': '*',
                'DBTEST_DIRS': '*',
                'DBTEST_ENV': '*',
                'DBTEST_LINK_LIBRARIES': '*',
                'UBENCH_DIRS': '*',
                'UBENCH_DATABASES': '*',
                'UBENCH_SOURCES': '*',
                'UBENCH_LINK_LIBRARIES': '*',
                'UBENCH_ENV': '*',
                'POSTGRES_TEST_DSN': '*',
            },
            'flags': ['NO_INSTALL', 'NO_CORE_LINK', 'GENERATE_DYNAMIC_CONFIGS'],
        },
        '_userver_directory_install': {
            'kwargs': {
                'COMPONENT': '*',
                'DIRECTORY': '*',
                'PROGRAMS': '*',
                'FILES': '*',
                'DESTINATION': '*',
            },
        },
        'userver_target_generate_openapi_client': {
            'pargs': 1,
            'kwargs': {
                'NAME': '*',
                'OUTPUT_DIR': '*',
                'SCHEMAS': '*',
                'UTEST_LINK_LIBRARIES': '*',
            },
        },
        'userver_target_generate_chaotic': {
            'pargs': 1,
            'kwargs': {
                'ARGS': '*',
                'FORMAT': '*',
                'OUTPUT_DIR': '*',
                'SCHEMAS': '*',
                'RELATIVE_TO': '*',
            },
            'flags': ['UNIQUE'],
        },
        'userver_venv_setup': {
            'kwargs': {
                'NAME': '*',
                'PYTHON_OUTPUT_VAR': '*',
                'REQUIREMENTS': '*',
            },
            'flags': ['UNIQUE'],
        },
        'userver_add_grpc_library': {
            'pargs': 1,
            'kwargs': {
                'PROTOS': '*',
                'INCLUDE_DIRECTORIES': '*',
            },
        },
        'userver_chaos_testsuite_add': {
            'kwargs': {
                'ENV': '*',
            },
        },
    }

# -----------------------------
# Options affecting formatting.
# -----------------------------
with section('format'):
    # Disable formatting entirely, making cmake-format a no-op
    disable = False

    # How wide to allow formatted cmake files
    line_width = 120

    # How many spaces to tab for indent
    tab_size = 4

    # If true, lines are indented using tab characters (utf-8 0x09) instead of
    # <tab_size> space characters (utf-8 0x20). In cases where the layout would
    # require a fractional tab character, the behavior of the  fractional
    # indentation is governed by <fractional_tab_policy>
    use_tabchars = False

    # If <use_tabchars> is True, then the value of this variable indicates how
    # fractional indentions are handled during whitespace replacement. If set to
    # 'use-space', fractional indentation is left as spaces (utf-8 0x20). If set
    # to `round-up` fractional indentation is replaced with a single tab character
    # (utf-8 0x09) effectively shifting the column to the next tabstop
    fractional_tab_policy = 'use-space'

    # If a statement is wrapped to more than one line, than dangle the closing
    # parenthesis on its own line.
    dangle_parens = True
