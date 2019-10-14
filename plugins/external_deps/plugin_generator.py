import os

import voluptuous

from codegen import plugin_manager
from codegen import utils


class RepositoryGenerator:
    config_schema = voluptuous.Schema({
        voluptuous.Required('name'): str,
        'package-name': str,
        'debian-names': [str],
        'formula-name': str,
        'version': voluptuous.Any(str, int),
        'includes': {
            'enabled': bool,
            'variable': str,
            'path-suffixes': list,
            'names': list,

        },
        'libraries': {
            'enabled': bool,
            'variable': str,
            'path-suffixes': list,
            'names': list,
        },
        'fail-message': str,
        'virtual': bool,
        'compile-definitions': {
            'names': list,
        },
    })

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

            manager.write(
                os.path.join(
                    cmake_generated_path, 'FindHelper{}.cmake'.format(key),
                ),
                manager.renderer.get_template('FindHelper.jinja').render({
                    'name': key,
                    'package_name': value.get('package-name'),
                    'debian_names': value.get('debian-names'),
                    'formula_name': value.get('formula-name'),
                    'version': value.get('version'),
                    'includes': value.get('includes'),
                    'libraries': value.get('libraries'),
                    'fail_message': fail_message,
                    'virtual': value.get('virtual', False),
                    'compile_definitions': value.get('compile-definitions'),
                }),
            )
