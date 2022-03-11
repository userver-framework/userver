import os

import voluptuous

from codegen import plugin_manager
from codegen import utils

FIND_HELPER_TYPE = 'find-helper'
EXTERNAL_PROJECT_TYPE = 'external-project'
SYSTEM_AND_EXTERNAL_TYPE = 'system-and-external'

EXTERNAL_DEPS_TYPE = [
    FIND_HELPER_TYPE,
    EXTERNAL_PROJECT_TYPE,
    SYSTEM_AND_EXTERNAL_TYPE,
]


class RepositoryGenerator:
    lib_schema = voluptuous.Schema(
        {
            voluptuous.Required('name'): str,
            'type': voluptuous.Any(None, *EXTERNAL_DEPS_TYPE),
            'package-name': str,
            'debian-names': [str],
            'debian-binary-depends': [str],
            'formula-name': str,
            'rpm-names': [str],
            'version': voluptuous.Any(str, int),
            voluptuous.Required('helper-prefix'): bool,
            'extra-cmake-vars': {str: str},
            'includes': {
                'enabled': bool,
                'variable': str,
                'find': [
                    {'path-suffixes': list, 'paths': list, 'names': list},
                ],
            },
            'libraries': {
                'enabled': bool,
                'variable': str,
                'find': [
                    {'path-suffixes': list, 'paths': list, 'names': list},
                ],
            },
            'programs': {
                'enabled': bool,
                'variable': str,
                'find': [
                    {'path-suffixes': list, 'paths': list, 'names': list},
                ],
            },
            'fail-message': str,
            'virtual': bool,
            'compile-definitions': {'names': list},
            'source': {'repository': str, 'tag': str, 'dir': str},
            'build-args': {'names': list},
            'depends': [str],
            'use-destdir': bool,
            'commands': {
                'list-separator': str,
                'patch': str,
                'download': str,
                'install': str,
                'update': str,
            },
            'log': {'download': bool, 'configure': bool, 'build': bool},
            'targets': [
                {
                    'name': str,
                    'includes': [str],
                    'libs': [str],
                    'depends': [str],
                },
            ],
            'checks': [{'expression': str, 'error': str}],
            'ya-make': voluptuous.Schema({}, extra=voluptuous.ALLOW_EXTRA),
        },
    )
    config_schema = voluptuous.Schema(
        voluptuous.Any(
            lib_schema,
            {
                voluptuous.Required('common-name'): str,
                voluptuous.Required('partials'): list,
            },
        ),
    )

    def __init__(self, config: dict) -> None:
        self.dependencies: dict = {}

    def configure(self, manager: plugin_manager.ConfigureManager) -> None:
        dependencies_dir = os.path.join(
            manager.params.root_dir, 'external-deps',
        )
        for path, _, files in os.walk(dependencies_dir):
            for file_name in files:
                _, config_ext = os.path.splitext(file_name)
                if config_ext != '.yaml':
                    continue
                file_path = os.path.join(path, file_name)
                config = utils.load_yaml(file_path)
                self.config_schema(config)
                if 'common-name' in config:
                    for partial in config.get('partials'):
                        self.dependencies[partial['name']] = {
                            'common-name': config['common-name'],
                            **partial,
                        }
                else:
                    self.dependencies[config['name']] = config

    def generate(self, manager: plugin_manager.GenerateManager) -> None:
        if not self.dependencies:
            return

        cmake_generated_path = os.path.join(
            manager.params.root_build_dir, 'cmake_generated',
        )
        # cmake_generated_path = manager.params.root_dir + '/cmake/generated'
        os.makedirs(cmake_generated_path, exist_ok=True)

        for key, value in self.dependencies.items():
            fail_message = value.get('fail-message')
            if fail_message:
                fail_message = fail_message.replace('\n', ' ')

            helper_prefix = 'Helper' if value['helper-prefix'] else ''
            filename = f'Find{helper_prefix}{key}.cmake'
            cmake_type = value.get('type', FIND_HELPER_TYPE)

            if value['helper-prefix'] and cmake_type == FIND_HELPER_TYPE:
                use_find = value.keys() & {'includes', 'libraries', 'programs'}
                if use_find:
                    raise RuntimeError(
                        f'{filename} would use '
                        f'"find_package_handle_standard_args" for "{key}" '
                        f'wich would cause a CMake "-Wdev" warning. '
                        f'To avoid that the external dep should have '
                        f'"helper-prefix: false" or have no following keys: '
                        f'{use_find}',
                    )

            if cmake_type in {FIND_HELPER_TYPE, SYSTEM_AND_EXTERNAL_TYPE}:
                manager.write(
                    os.path.join(cmake_generated_path, filename),
                    manager.renderer.get_template('FindHelper.jinja').render(
                        {
                            'name': key,
                            'package_name': value.get('package-name'),
                            'common_name': value.get('common-name'),
                            'debian_names': value.get('debian-names'),
                            'formula_name': value.get('formula-name'),
                            'rpm_names': value.get('rpm-names'),
                            'version': value.get('version'),
                            'extra_cmake_vars': value.get(
                                'extra-cmake-vars', {},
                            ),
                            'includes': value.get('includes'),
                            'libraries': value.get('libraries'),
                            'programs': value.get('programs'),
                            'fail_message': fail_message,
                            'virtual': value.get('virtual', False),
                            'compile_definitions': value.get(
                                'compile-definitions',
                            ),
                            'checks': value.get('checks', []),
                        },
                    ),
                )

            external_prefix = ''
            if value.get('type') == SYSTEM_AND_EXTERNAL_TYPE:
                external_prefix = 'External'

            filename = f'Find{external_prefix}{helper_prefix}{key}.cmake'
            if value.get('type') in {
                    EXTERNAL_PROJECT_TYPE,
                    SYSTEM_AND_EXTERNAL_TYPE,
            }:
                if 'repository' in value.get('source', {}):
                    log = value.setdefault('log', {})
                    if 'configure' not in log:
                        log['configure'] = False

                if 'dir' in value.get('source', {}):
                    value['source']['dir'] = os.path.join(
                        '${USERVER_ROOT_DIR}', value['source']['dir'],
                    )

                manager.write(
                    os.path.join(cmake_generated_path, filename),
                    manager.renderer.get_template(
                        'FindExternalProject.jinja',
                    ).render(
                        {
                            'name': key,
                            'source': value.get('source', {}),
                            'build_args': value.get('build-args', []),
                            'use_destdir': value.get('use-destdir', False),
                            'commands': value.get('commands', {}),
                            'log': value.get('log', {}),
                            'compile_definitions': value.get(
                                'compile-definitions',
                            ),
                            'targets': value.get('targets', {}),
                            'depends': value.get('depends', []),
                        },
                    ),
                )
