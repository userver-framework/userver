#!/usr/bin/python

import argparse
import os
import pathlib
from typing import Any

import yaml

USERVER_ROOT = pathlib.Path(__file__).parent.parent.parent
DEFAULT_DESCRIPTION_TAG = '\nDefault:'


def normalize_default_value(yaml_node: dict) -> str:
    data = yaml_node.get('default')
    if data is None and DEFAULT_DESCRIPTION_TAG in yaml_node.get('description'):
        data = yaml_node.get('description').rsplit(DEFAULT_DESCRIPTION_TAG, 1)[-1]

    if data is True:
        return 'true'
    elif data is False:
        return 'false'
    elif data is None:
        return '--'
    else:
        data = str(data).replace('\n', ' ').replace('\r', '').replace('<', '&lt;').replace('>', '&gt;').strip()
        if data == '':
            return '""'
        return data


def merge_descriptions(yaml_node: dict, other: dict) -> dict:
    if not yaml_node.get('description') and not yaml_node.get('default'):
        return other

    if not other.get('description') and not other.get('default'):
        return yaml_node

    value_merged = yaml_node.copy()

    if yaml_node.get('description') and other.get('description'):
        description = yaml_node['description'].strip()

        if not description.endswith('.'):
            description += '.'

        if DEFAULT_DESCRIPTION_TAG in description:
            description = description.replace(DEFAULT_DESCRIPTION_TAG, ' <i>Each of the elements:</i> ' + other['description'] + DEFAULT_DESCRIPTION_TAG)
        else:
            description += ' <i>Each of the elements:</i> ' + other['description']

        value_merged['description'] = description

    for field in ('description', 'default'):
        if not yaml_node.get(field) and other.get(field):
            value_merged[field] = other[field]

    return value_merged


def enrich_description(yaml_node: dict) -> str:
    description = yaml_node['description']

    if DEFAULT_DESCRIPTION_TAG in description:
        description = description.rsplit(DEFAULT_DESCRIPTION_TAG, 1)[0]

    description = description.replace('\n', ' ').replace('\r', '').strip()
    description = f'{description[0].upper()}{description[1:]}'
    if not description.endswith('.'):
        description += '.'

    if enum := yaml_node.get('enum'):
        description += ' <i>Possible values:</i> ' + ', '.join(str(x) for x in enum) + '.'

    return description


def visit_object(yaml_node: dict, prefix: str = ''):
    properties = yaml_node.get('properties', {})

    additionals = yaml_node.get('additionalProperties')
    if not additionals and yaml_node.get('description') and prefix:
        yield ('__' + prefix.removesuffix('.') + '__', yaml_node)

    for key, value in properties.items():
        if value.get('type') == 'array':
            if value['items'].get('type') == 'object':
                yield from visit_object(value['items'], prefix + key + '.[].')
            else:
                value_merged = merge_descriptions(value, value['items'])
                yield (prefix + key + '.[]', value_merged)
        elif value.get('type') == 'object':
            yield from visit_object(value, prefix + key + '.')
        else:
            yield (prefix + key, value)

    additionals = yaml_node.get('additionalProperties')
    if additionals is True:
        yield (prefix + '*', yaml_node)
    elif not additionals:
        return
    elif additionals.get('type') == 'object':
        yield from visit_object(yaml_node['additionalProperties'], prefix + '*.')
    elif additionals.get('type') == 'array':
        value_merged = merge_descriptions(yaml_node, additionals['items'])
        yield (prefix + '*.[]', value_merged)
    else:
        value_merged = merge_descriptions(yaml_node, additionals)
        yield (prefix + '*', value_merged)


def format_schema(yaml_node: Any) -> str:
    result = ''
    key_values = list(visit_object(yaml_node))
    if not key_values:
        result += 'No options'
        return result

    max_key_size = max(len(key) for key, _ in key_values)
    max_value_size = max(len(enrich_description(value)) for _, value in key_values)

    result += f'{"Name":{max_key_size}} | {"Description":{max_value_size}} | Default value\n'
    result += f'{"":-<{max_key_size}} | {"":-<{max_value_size}} | {"":-<16}\n'

    for key, value in key_values:
        key = key.replace('\n', ' ').replace('\r', '')
        description = enrich_description(value)
        default = normalize_default_value(value)
        result += f'{key:{max_key_size}} | {description:{max_value_size}} | {default}\n'

    return result


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o')
    parser.add_argument('yaml', nargs='+')
    return parser.parse_args()


def handle_file(ifname: pathlib.Path, output_dir: pathlib.Path) -> None:
    with open(ifname) as ifile:
        content = yaml.load(ifile.read(), yaml.Loader)

    md = format_schema(content)

    output_path = output_dir / ifname.parent.relative_to(USERVER_ROOT)
    os.makedirs(str(output_path), exist_ok=True)

    output_file = output_path / (pathlib.Path(ifname).stem + '.md')
    print(output_file)

    with open(output_file, 'w') as ofile:
        ofile.write(md)


def main():
    args = parse_args()
    for fname in args.yaml:
        try:
            handle_file(pathlib.Path(fname), pathlib.Path(args.o))
        except yaml.scanner.ScannerError as err:
            print(f'Failed to parse YAML from file {fname}: {err}')


if __name__ == '__main__':
    main()
