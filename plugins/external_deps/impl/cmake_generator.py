import argparse
import os

import jinja2
import voluptuous
import yaml


FIND_HELPER_TYPE = 'find-helper'
EXTERNAL_PROJECT_TYPE = 'external-project'

EXTERNAL_DEPS_TYPE = [FIND_HELPER_TYPE, EXTERNAL_PROJECT_TYPE]

LIB_SCHEMA = voluptuous.Schema(
    {
        voluptuous.Required('name'): str,
        'type': voluptuous.Any(None, *EXTERNAL_DEPS_TYPE),
        'package-name': str,
        'debian-names': [str],
        'debian-binary-depends': [str],
        'formula-name': str,
        'rpm-names': [str],
        'pacman-names': [str],
        'version': voluptuous.Any(str, int),
        voluptuous.Required('helper-prefix'): bool,
        'extra-cmake-vars': {str: str},
        'includes': {
            'enabled': bool,
            'variable': str,
            'find': [{'path-suffixes': list, 'paths': list, 'names': list}],
        },
        'libraries': {
            'enabled': bool,
            'variable': str,
            'find': [{'path-suffixes': list, 'paths': list, 'names': list}],
        },
        'programs': {
            'enabled': bool,
            'variable': str,
            'find': [{'path-suffixes': list, 'paths': list, 'names': list}],
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
            {'name': str, 'includes': [str], 'libs': [str], 'depends': [str]},
        ],
        'checks': [{'expression': str, 'error': str}],
        'ya-make': voluptuous.Schema({}, extra=voluptuous.ALLOW_EXTRA),
    },
)
CONFIG_SCHEMA = voluptuous.Schema(
    voluptuous.Any(
        LIB_SCHEMA,
        {
            voluptuous.Required('common-name'): str,
            voluptuous.Required('partials'): list,
        },
    ),
)


def _load_yaml(path):
    with open(path, encoding='utf-8') as fin:
        return yaml.load(fin, getattr(yaml, 'CSafeLoader', yaml.SafeLoader))


def generate_cmake(name: str, value, renderer: jinja2.Environment):
    result: dict = {}

    fail_message = value.get('fail-message')
    if fail_message:
        fail_message = fail_message.replace('\n', ' ')

    helper_prefix = 'Helper' if value['helper-prefix'] else ''
    filename = f'Find{helper_prefix}{name}.cmake'
    cmake_type = value.get('type', FIND_HELPER_TYPE)

    if value['helper-prefix'] and cmake_type == FIND_HELPER_TYPE:
        use_find = value.keys() & {'includes', 'libraries', 'programs'}
        if use_find:
            raise RuntimeError(
                f'{filename} would use '
                f'"find_package_handle_standard_args" for "{name}" '
                'wich would cause a CMake "-Wdev" warning. '
                'To avoid that the external dep should have '
                '"helper-prefix: false" or have no following keys: '
                f'{use_find}',
            )

    if cmake_type == FIND_HELPER_TYPE:
        result[filename] = renderer.get_template('FindHelper.jinja').render(
            {
                'name': name,
                'package_name': value.get('package-name'),
                'common_name': value.get('common-name'),
                'debian_names': value.get('debian-names'),
                'formula_name': value.get('formula-name'),
                'rpm_names': value.get('rpm-names'),
                'pacman_names': value.get('pacman-names'),
                'version': value.get('version'),
                'extra_cmake_vars': value.get('extra-cmake-vars', {}),
                'includes': value.get('includes'),
                'libraries': value.get('libraries'),
                'programs': value.get('programs'),
                'fail_message': fail_message,
                'virtual': value.get('virtual', False),
                'compile_definitions': value.get('compile-definitions'),
                'checks': value.get('checks', []),
            },
        )

    filename = f'Find{helper_prefix}{name}.cmake'
    if value.get('type') == EXTERNAL_PROJECT_TYPE:
        if 'repository' in value.get('source', {}):
            log = value.setdefault('log', {})
            if 'configure' not in log:
                log['configure'] = False

        if 'dir' in value.get('source', {}):
            value['source']['dir'] = os.path.join(
                '${USERVER_ROOT_DIR}', value['source']['dir'],
            )

        result[filename] = (
            renderer.get_template('FindExternalProject.jinja').render(
                {
                    'name': name,
                    'source': value.get('source', {}),
                    'build_args': value.get('build-args', []),
                    'use_destdir': value.get('use-destdir', False),
                    'commands': value.get('commands', {}),
                    'log': value.get('log', {}),
                    'compile_definitions': value.get('compile-definitions'),
                    'targets': value.get('targets', {}),
                    'depends': value.get('depends', []),
                },
            )
        )

    return result


def parse_deps_files(dependencies_dir: str) -> dict:
    dependencies: dict = {}

    for path, _, files in os.walk(dependencies_dir):
        for file_name in files:
            _, config_ext = os.path.splitext(file_name)
            if config_ext != '.yaml':
                continue
            file_path = os.path.join(path, file_name)
            config = _load_yaml(file_path)
            CONFIG_SCHEMA(config)
            if 'common-name' in config:
                for partial in config.get('partials'):
                    dependencies[partial['name']] = {
                        'common-name': config['common-name'],
                        **partial,
                    }
            else:
                dependencies[config['name']] = config

    return dependencies


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo-dir', required=True)
    parser.add_argument('--build-dir', required=True)
    parser.add_argument('--log-level', default='WARNING')

    args = parser.parse_args(argv)

    dependencies_dir = os.path.join(args.repo_dir, 'external-deps')
    dependencies = parse_deps_files(dependencies_dir)

    cmake_generated_path = os.path.join(args.build_dir, 'cmake_generated')
    os.makedirs(cmake_generated_path, exist_ok=True)

    template_path = os.path.join(
        args.repo_dir, 'plugins', 'external_deps', 'templates',
    )
    renderer = jinja2.Environment(
        trim_blocks=True,
        keep_trailing_newline=True,
        loader=jinja2.FileSystemLoader([template_path]),
        undefined=jinja2.StrictUndefined,
        extensions=['jinja2.ext.loopcontrols'],
    )
    for key, value in dependencies.items():
        for filename, data in generate_cmake(key, value, renderer).items():
            file_path = os.path.join(cmake_generated_path, filename)
            with open(file_path, 'w') as output_file:
                output_file.write(data)


if __name__ == '__main__':
    main()
