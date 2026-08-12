"""Deep-merge multiple YAML config files into one.

Later files win on scalar conflicts. Mappings are merged recursively.
Usage: merge_yaml_configs.py INPUT [INPUT ...] -o OUTPUT
"""

import argparse
import sys

import yaml


def _deep_merge(base: dict, extra: dict) -> dict:
    result = dict(base)
    for key, value in extra.items():
        if key in result and isinstance(result[key], dict) and isinstance(value, dict):
            result[key] = _deep_merge(result[key], value)
        else:
            result[key] = value
    return result


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs', nargs='+', metavar='INPUT')
    parser.add_argument('-o', '--output', required=True)
    return parser.parse_args()


def main() -> None:
    args = _parse_args()

    merged: dict = {}
    for path in args.inputs:
        with open(path) as f:
            content = yaml.safe_load(f)
        if content is None:
            continue
        if not isinstance(content, dict):
            print(f'error: {path}: expected a YAML mapping', file=sys.stderr)
            sys.exit(1)
        merged = _deep_merge(merged, content)

    with open(args.output, 'w') as f:
        yaml.dump(merged, f, default_flow_style=False, sort_keys=False)


if __name__ == '__main__':
    main()
