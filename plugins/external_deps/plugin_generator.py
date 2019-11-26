import os

import voluptuous

from codegen import plugin_manager
from codegen import utils

FIND_HELPER_TYPE = 'find-helper'
EXTERNAL_PROJECT_TYPE = 'external-project'

EXTERNAL_DEPS_TYPE = [FIND_HELPER_TYPE, EXTERNAL_PROJECT_TYPE]


class RepositoryGenerator:
    lib_schema = voluptuous.Schema(
        {
            voluptuous.Required('name'): str,
            'type': voluptuous.Any(None, *EXTERNAL_DEPS_TYPE),
            'package-name': str,
            'debian-names': [str],
            'debian-binary-depends': [str],
            'formula-name': str,
            'version': voluptuous.Any(str, int),
            'helper-prefix': bool,
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
        },
    )
    config_schema = voluptuous.Schema(
        voluptuous.Any(
            {
                voluptuous.Required('common-name'): str,
                voluptuous.Required('partials'): list,
            },
            lib_schema,
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
        os.makedirs(cmake_generated_path, exist_ok=True)

        for key, value in self.dependencies.items():
            fail_message = value.get('fail-message')
            if fail_message:
                fail_message = fail_message.replace('\n', ' ')

            filename = 'Find{}{}.cmake'.format(
                'Helper' if value.get('helper-prefix', True) else '', key,
            )
            if value.get('type', FIND_HELPER_TYPE) == FIND_HELPER_TYPE:
                manager.write(
                    os.path.join(cmake_generated_path, filename),
                    manager.renderer.get_template('FindHelper.jinja').render(
                        {
                            'name': key,
                            'package_name': value.get('package-name'),
                            'common_name': value.get('common-name'),
                            'debian_names': value.get('debian-names'),
                            'formula_name': value.get('formula-name'),
                            'version': value.get('version'),
                            'includes': value.get('includes'),
                            'libraries': value.get('libraries'),
                            'programs': value.get('programs'),
                            'fail_message': fail_message,
                            'virtual': value.get('virtual', False),
                            'compile_definitions': value.get(
                                'compile-definitions',
                            ),
                            'checks': value.get('checks'),
                        },
                    ),
                )
            elif value.get('type') == EXTERNAL_PROJECT_TYPE:
                if 'repository' in value.get('source', {}):
                    commands = value.setdefault('commands', {})
                    if 'install' not in commands:
                        value['use-destdir'] = True
                        commands['install'] = 'make DESTDIR=${DESTDIR} install'

                    log = value.setdefault('log', {})
                    if 'configure' not in log:
                        log['configure'] = False

                if 'dir' in value.get('source', {}):
                    value['source']['dir'] = os.path.join(
                        manager.params.root_dir, value['source']['dir'],
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
