#!/usr/bin/python3

import os
import pathlib
import subprocess
from typing import List

import jinja2
import library.python.resource as arc_resource
from taxi_linters import taxi_yamlfmt


def arc_root() -> pathlib.Path:
    out = subprocess.check_output(['arc', 'root'], encoding='utf-8')
    return pathlib.Path(out.strip())


def dynamic_config_paths(selected: List[str]):
    declarations = arc_root() / 'taxi' / 'schemas' / 'schemas' / 'configs' / 'declarations'
    for dirpath, _, filenames in os.walk(declarations):
        for filename in filenames:
            if filename.split('.')[0] in selected:
                yield os.path.join(dirpath, filename)


def userver_root() -> pathlib.Path:
    return arc_root() / 'taxi' / 'uservices' / 'userver'


def userver_modules() -> List[str]:
    return [item for item in userver_root().iterdir() if item.is_dir() and (item / 'library.yaml').exists()]


def read_configs_names(module) -> List[str]:
    with open(module / 'library.yaml') as ifile:
        content = taxi_yamlfmt.load(ifile)
        return content.get('configs', {}).get('names', [])


def make_arcadia_env():
    def arc_resource_loader(name: str) -> jinja2.BaseLoader:
        return arc_resource.resfs_read(f'{name}').decode('utf-8')

    loader = jinja2.FunctionLoader(arc_resource_loader)
    return jinja2.Environment(loader=loader)


def is_russian(string: str) -> bool:
    for char in string:
        if 'а' <= char <= 'я' or 'А' <= char <= 'Я':
            return True
    return False


def erase_russian_description(data) -> None:
    if isinstance(data, dict):
        if is_russian(data.get('description', '')):
            del data['description']
        for value in data.values():
            erase_russian_description(value)
    if isinstance(data, list):
        for value in data:
            erase_russian_description(value)


def handle_config_file(filepath: pathlib.Path, module: pathlib.Path) -> None:
    with open(filepath) as ifile:
        old_content = taxi_yamlfmt.load(ifile)
        description = old_content['description']
        if is_russian(description):
            description = ''
        erase_russian_description(old_content['schema'])
        content = {
            'default': old_content['default'],
            'description': description,
            'schema': old_content['schema'],
        }

    (module / 'dynamic_configs').mkdir(exist_ok=True)
    with open(module / 'dynamic_configs' / filepath.name, 'w') as ofile:
        taxi_yamlfmt.dump(content, ofile)


def handle_ya_make(module: pathlib.Path, config_names: List[str]) -> None:
    env = make_arcadia_env()
    content = env.get_template('ya.make.jinja').render(
        config_names=config_names,
        module=module.stem,
    )
    with open(module / 'dynamic_configs' / 'ya.make.dynconfigs', 'w') as ofile:
        ofile.write(content)


def main():
    for module in userver_modules():
        print(module)
        configs = read_configs_names(module)
        config_names = []
        for filepath in dynamic_config_paths(configs):
            handle_config_file(pathlib.Path(filepath), module)
            config_names.append(pathlib.Path(filepath).stem)
        if configs:
            handle_ya_make(pathlib.Path(module), config_names)


if __name__ == '__main__':
    main()
