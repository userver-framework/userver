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
            'flags': [
                'EXCLUDE_FROM_ALL',
                'DOWNLOAD_ONLY',
                'SYSTEM',
            ],
            'kwargs': {
                'NAME': '*',
                'VERSION': '*',
                'GITHUB_REPOSITORY': '*',
                'GIT_SHALLOW': '*',
                'URL': '*',
                'OPTIONS': '*',
                'PATCHES': '*',
                'SOURCE_SUBDIR': '*',
                'GIT_TAG': '*',
            },
        },
        'userver_module': {
            'pargs': 1,
            'flags': [
                'NO_INSTALL',
                'NO_CORE_LINK',
                'GENERATE_DYNAMIC_CONFIGS',
            ],
            'kwargs': {
                'SOURCE_DIR': '*',
                'INSTALL_COMPONENT': '*',
                'IGNORE_SOURCES': '*',
                'LINK_LIBRARIES': '*',
                'LINK_LIBRARIES_PRIVATE': '*',
                'INCLUDE_DIRS': '*',
                'INCLUDE_DIRS_PRIVATE': '*',
                'UTEST_DIRS': '*',
                'UTEST_SOURCES': '*',
                'UTEST_LINK_LIBRARIES': '*',
                'DBTEST_DIRS': '*',
                'DBTEST_SOURCES': '*',
                'DBTEST_DATABASES': '*',
                'DBTEST_ENV': '*',
                'DBTEST_LINK_LIBRARIES': '*',
                'UBENCH_DIRS': '*',
                'UBENCH_SOURCES': '*',
                'UBENCH_LINK_LIBRARIES': '*',
                'UBENCH_DATABASES': '*',
                'UBENCH_ENV': '*',
                'COMPONENT_DEPENDS': '*',
                'EMBED_FILES': '*',
            },
        },
        '_userver_directory_install': {
            'kwargs': {
                'COMPONENT': '*',
                'DESTINATION': '*',
                'FILES': '*',
                'DIRECTORY': '*',
                'PROGRAMS': '*',
                'PATTERN': '*',
                'RENAME': '*',
                'EXCLUDE_PATTERNS': '*',
            },
        },
        'userver_target_generate_openapi_client': {
            'pargs': 1,
            'kwargs': {
                'OUTPUT_DIR': '*',
                'NAME': '*',
                'FORMAT': '*',
                'SCHEMAS': '*',
                'ARGS': '*',
            },
        },
        'userver_target_generate_chaotic': {
            'pargs': 1,
            'flags': [
                'GENERATE_SERIALIZERS',
                'PARSE_EXTRA_FORMATS',
                'NO_SAX_PARSE',
                'NO_STREAM_WRITER',
            ],
            'kwargs': {
                'OUTPUT_DIR': '*',
                'RELATIVE_TO': '*',
                'FORMAT': '*',
                'INSTALL_INCLUDES_COMPONENT': '*',
                'OUTPUT_PREFIX': '*',
                'ERASE_PATH_PREFIX': '*',
                'SCHEMAS': '*',
                'LAYOUT': '*',
                'INCLUDE_DIRS': '*',
                'LINK_TARGETS': '*',
            },
        },
        'userver_testsuite_requirements': {
            'flags': [
                'TESTSUITE_ONLY',
            ],
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
                'RESOURCE_LOCKS': '*',
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
                'RESOURCE_LOCKS': '*',
            },
        },
        'userver_add_utest': {
            'flags': [
                'DISABLE_GTEST_XML_OUTPUT',
            ],
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
            'flags': [
                'UNIQUE',
            ],
            'kwargs': {
                'NAME': '*',
                'PYTHON_OUTPUT_VAR': '*',
                'REQUIREMENTS': '*',
                'PIP_ARGS': '*',
            },
        },
        'userver_add_grpc_library': {
            'pargs': 1,
            'kwargs': {
                'SOURCE_PATH': '*',
                'OUTPUT_PATH': '*',
                'PROTOS': '*',
                'INCLUDE_DIRECTORIES': '*',
            },
        },
        'userver_chaos_testsuite_add': {
            'kwargs': {
                'TESTS_DIRECTORY': '*',
                'PYTHONPATH': '*',
                'ENV': '*',
                'RESOURCE_LOCKS': '*',
            },
        },
        '_userver_module_begin': {
            'flags': [
                'CPM_DOWNLOAD_ONLY',
            ],
            'kwargs': {
                'NAME': '*',
                'VERSION': '*',
                'DEBIAN_NAMES': '*',
                'FORMULA_NAMES': '*',
                'RPM_NAMES': '*',
                'PACMAN_NAMES': '*',
                'PKG_NAMES': '*',
                'PKG_CONFIG_NAMES': '*',
                'CPM_NAME': '*',
                'CPM_VERSION': '*',
                'CPM_GITHUB_REPOSITORY': '*',
                'CPM_URL': '*',
                'CPM_OPTIONS': '*',
                'CPM_SOURCE_SUBDIR': '*',
                'CPM_GIT_TAG': '*',
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
            'flags': [
                'OPTIONAL',
            ],
            'kwargs': {
                'NAMES': '*',
                'PATHS': '*',
                'PATH_SUFFIXES': '*',
            },
        },
        '_userver_module_find_program': {
            'kwargs': {
                'NAMES': '*',
                'PATHS': '*',
                'PATH_SUFFIXES': '*',
            },
        },
        '_userver_module_find_part': {
            'flags': [
                'OPTIONAL',
            ],
            'kwargs': {
                'PART_TYPE': '*',
                'NAMES': '*',
                'PATH_SUFFIXES': '*',
                'PATHS': '*',
            },
        },
        '_userver_install_targets': {
            'kwargs': {
                'COMPONENT': '*',
                'TARGETS': '*',
            },
        },
        '_userver_install_component': {
            'flags': [
                'NON_FINDABLE',
            ],
            'kwargs': {
                'COMPONENT': '*',
                'DEPENDS': '*',
            },
        },
        'userver_embed_file': {
            'pargs': 1,
            'kwargs': {
                'NAME': '*',
                'FILEPATH': '*',
                'HPP_FILENAME': '*',
            },
        },
        'userver_generate_grpc_files': {
            'kwargs': {
                'CPP_FILES': '*',
                'CPP_USRV_FILES': '*',
                'GENERATED_INCLUDES': '*',
                'SOURCE_PATH': '*',
                'OUTPUT_PATH': '*',
                'PROTOS': '*',
                'INCLUDE_DIRECTORIES': '*',
            },
        },
        'userver_target_generate_openapi_handlers': {
            'pargs': 1,
            'kwargs': {
                'NAME': '*',
                'OUTPUT_DIR': '*',
                'SRC_DIR': '*',
                'FORMAT': '*',
                'SCHEMAS': '*',
                'ARGS': '*',
            },
        },
        'userver_generate_config_yaml': {
            'pargs': 1,
            'kwargs': {
                'OUTPUT': '*',
                'BASE_CONFIGS': '*',
            },
        },
        'userver_add_sql_library': {
            'pargs': 1,
            'kwargs': {
                'SOURCE_DIR': '*',
                'OUTPUT_DIR': '*',
                'NAMESPACE': '*',
                'QUERY_LOG_MODE': '*',
                'DTO_DIALECT': '*',
                'MIGRATIONS_DIR': '*',
                'DUMP_DIR': '*',
                'SQL_FILES': '*',
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


# ------------------------------------------------
# Options affecting comment reflow and formatting.
# ------------------------------------------------
with section('markup'):
    # enable comment markup parsing and reflow
    enable_markup = False
