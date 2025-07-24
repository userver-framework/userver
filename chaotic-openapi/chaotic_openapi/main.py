import argparse
import sys

import yaml

from chaotic import error as chaotic_error
from chaotic_openapi.back.cpp_client import renderer
from chaotic_openapi.back.cpp_client import translator
from chaotic_openapi.front import parser as front_parser
from chaotic_openapi.front import ref_resolver


def main():
    try:
        do_main()
    except chaotic_error.BaseError as exc:
        print(exc, file=sys.stderr)
        sys.exit(1)


def do_main():
    args = parse_args()

    # sort
    contents = {}
    for file in args.file:
        with open(file) as ifile:
            content = yaml.safe_load(ifile)
        contents[file] = content
    sorted_contents = ref_resolver.sort_openapis(contents)

    # parse
    parser = front_parser.Parser(args.name)
    for file, content in sorted_contents:
        parser.parse_schema(content, file)

    # translate
    spec = translator.Translator(
        parser.service(),
        dynamic_config=args.dynamic_config,
        cpp_namespace=(args.namespace or f'clients::{args.name}'),
        include_dirs=[],
    ).spec()

    # render
    ctx = renderer.Context(
        generate_path='',
        clang_format_bin=args.clang_format,
        uservices_library_tvm_guard_hack=False,
    )
    outputs = renderer.render(spec, ctx)
    renderer.CppOutput.save(outputs, args.output_dir)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', required=True, help='Client name')
    parser.add_argument('-o', '--output-dir', required=True)
    parser.add_argument('--namespace', required=False)
    parser.add_argument('--dynamic-config', default='')
    parser.add_argument(
        '--clang-format',
        default='clang-format',
        help=(
            'clang-format binary name. Can be either binary name in $PATH or '
            'full filepath to a binary file. Set to empty for no formatting.'
        ),
    )
    parser.add_argument(
        'file',
        nargs='+',
        help='openapi/swagger yaml/json schemas',
    )
    return parser.parse_args()


if __name__ == '__main__':
    main()
