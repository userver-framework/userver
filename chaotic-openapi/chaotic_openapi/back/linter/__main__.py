import argparse  # noqa: I001
import json
import pprint

import yaml

from chaotic_openapi.front import parser as front_parser
from . import validators


def parse_args():
    parser = argparse.ArgumentParser(description='OpenApi/Swagger linter for Yandex')
    parser.add_argument('file', type=str, nargs='+')
    return parser.parse_args()


def main():
    args = parse_args()

    parser = front_parser.Parser('test')
    for file in args.file:
        process_file(parser, file, args)
    validators.validate(parser.service())


def process_file(parser: front_parser.Parser, file, args):
    with open(file) as ifile:
        if file.endswith('.json'):
            content = json.load(ifile)
        else:
            content = yaml.safe_load(ifile)

    parser.parse_schema(content, file)

    pprint.pprint(parser.service())


if __name__ == '__main__':
    main()
