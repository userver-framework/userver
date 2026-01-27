# ----------------------------------
# Options affecting listfile parsing
# ----------------------------------
with section('parse'):  # noqa: F821
    # Specify structure for custom cmake functions
    # flags - single word flags
    # pargs - positional arguments
    # kwargs - keyword arguments
    additional_commands = {
        'cpmaddpackage': {
            'flags': ['EXCLUDE_FROM_ALL', 'DOWNLOAD_ONLY', 'SYSTEM'],
            'kwargs': {
                'NAME': '*',
                'VERSION': '*',
                'GITHUB_REPOSITORY': '*',
                'URL': '*',
                'OPTIONS': '*',
                'PATCHES': '*',
                'SOURCE_SUBDIR': '*',
                'GIT_TAG': '*',
                'GIT_SHALLOW': '*',
            },
        },
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
                'DEPENDS': '*',
                'EMBED_FILES': '*',
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
                'RENAME': '*',
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
        'userver_testsuite_requirements': {
            'flags': ['TESTSUITE_ONLY'],
            'kwargs': {
                'REQUIREMENTS_FILES_VAR': '*',
            },
        },
        'userver_testsuite_add': {
            'kwargs': {
                'SERVICE_TARGET': '*',
                'TEST_SUFFIX': '*',
                'WORKING_DIRECTORY': '*',
                'PYTHON_BINARY': '*',
                'PRETTY_LOGS': '*',
                'SQL_LIBRARY': '*',
                'PYTEST_ARGS': '*',
                'REQUIREMENTS': '*',
                'PYTHONPATH': '*',
                'TEST_ENV': '*',
            },
        },
        'userver_testsuite_add_simple': {
            'kwargs': {
                'SERVICE_TARGET': '*',
                'TEST_SUFFIX': '*',
                'WORKING_DIRECTORY': '*',
                'PYTHON_BINARY': '*',
                'PRETTY_LOGS': '*',
                'CONFIG_PATH': '*',
                'CONFIG_VARS_PATH': '*',
                'DYNAMIC_CONFIG_FALLBACK_PATH': '*',
                'SECDIST_PATH': '*',
                'DUMP_CONFIG': '*',
                'SQL_LIBRARY': '*',
                'PYTEST_ARGS': '*',
                'REQUIREMENTS': '*',
                'PYTHONPATH': '*',
                'TEST_ENV': '*',
            },
        },
        'userver_add_utest': {
            'flags': ['DISABLE_GTEST_XML_OUTPUT'],
            'kwargs': {
                'NAME': '*',
                'DATABASES': '*',
                'TEST_ENV': '*',
                'TEST_ARGS': '*',
            },
        },
        'userver_add_ubench_test': {
            'kwargs': {
                'NAME': '*',
                'DATABASES': '*',
                'TEST_ENV': '*',
            },
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
        '_userver_module_begin': {
            'flags': ['CPM_DOWNLOAD_ONLY'],
            'kwargs': {
                'NAME': '*',
                'DEBIAN_NAMES': '*',
                'FORMULA_NAMES': '*',
                'RPM_NAMES': '*',
                'PKG_NAMES': '*',
                'PACMAN_NAMES': '*',
                'PKG_CONFIG_NAMES': '*',
                'VERSION': '*',
                'CPM_NAME': '*',
                'CPM_GIT_TAG': '*',
                'CPM_URL': '*',
                'CPM_GITHUB_REPOSITORY': '*',
                'CPM_VERSION': '*',
                'CPM_OPTIONS': '*',
            },
        },
        '_userver_module_find_include': {
            'kwargs': {
                'NAME': '*',
                'PATHS': '*',
                'PATH_SUFFIXES': '*',
            },
        },
        '_userver_module_find_library': {
            'flags': ['OPTIONAL'],
            'kwargs': {
                'NAMES': '*',
                'PATHS': '*',
            },
        },
        '_userver_module_find_part': {
            'flags': ['OPTIONAL'],
            'kwargs': {
                'PART_TYPE': '*',
                'NAMES': '*',
                'PATH_SUFFIXES': '*',
                'PATHS': '*',
            },
        },
    }

# -----------------------------
# Options affecting formatting.
# -----------------------------
with section('format'):  # noqa: F821
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
