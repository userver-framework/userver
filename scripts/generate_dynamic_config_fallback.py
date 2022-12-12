#!/usr/bin/python3

import json
import os
import pathlib
from typing import Any
from typing import Dict
from typing import List
from typing import NamedTuple

import yaml


USERVER_ROOT = pathlib.Path(__file__).parent.parent
SCHEMAS = (
    USERVER_ROOT
    / '..'
    / '..'
    / 'schemas'
    / 'schemas'
    / 'configs'
    / 'declarations'
)


class Group(NamedTuple):
    library_yaml_filenames: List[str] = []
    dest_fallback_filenames: List[str] = []
    overrides: Dict[str, Any] = {}


class Config(NamedTuple):
    groups: List[Group] = []


def read_config(filepath: pathlib.Path) -> Config:
    with open(filepath, 'r') as ffi:
        contents = yaml.safe_load(ffi)

    config = Config()
    groups = contents['groups']
    for group in groups:
        group = Group(
            library_yaml_filenames=group['library-yamls'],
            dest_fallback_filenames=group['fallbacks'],
            overrides=group.get('overrides', {}),
        )
        config.groups.append(group)

    return config


class Loader:
    def __init__(self, path: str) -> None:
        self.path = path

    def load(self) -> Any:
        with open(self.path) as ffi:
            contents = yaml.safe_load(ffi)
        default = contents['default']
        return default


def make_loader(path: str) -> Loader:
    return Loader(path=path)


def read_fallbacks(schemas: pathlib.Path) -> Dict[str, Any]:
    fallbacks = {}
    for root, _dirs, files in os.walk(schemas):
        for file in files:
            if not file.endswith('.yaml'):
                continue

            name, _ext = file.rsplit('.', 2)

            path = os.path.join(root, file)
            fallbacks[name] = make_loader(path)
    return fallbacks


def write_fallbacks(config: Config, fallbacks: Dict[str, Any]) -> None:
    for group in config.groups:
        configs_names = []
        for library_yaml in group.library_yaml_filenames:
            with open(USERVER_ROOT / library_yaml) as ffi:
                contents = yaml.safe_load(ffi)
            configs_names += contents.get('configs', {}).get('names', [])

        configs = {}
        for name in configs_names:
            configs[name] = fallbacks[name].load()

        for name, value in group.overrides.items():
            configs[name] = value

        for fallback in group.dest_fallback_filenames:
            with open(USERVER_ROOT / fallback, 'w') as ffo:
                contents = json.dumps(configs, sort_keys=True, indent='  ')
                ffo.write(contents + '\n')


def main() -> None:
    configs = read_config(USERVER_ROOT / 'dynamic_config_fallbacks.yaml')
    fallbacks = read_fallbacks(SCHEMAS)
    write_fallbacks(configs, fallbacks)


if __name__ == '__main__':
    main()
