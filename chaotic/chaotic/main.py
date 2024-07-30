import argparse
import dataclasses
import os
import re
import sys
from typing import Any
from typing import Callable
from typing import Dict
from typing import List
from typing import Optional

import yaml

from chaotic.back.cpp import renderer
from chaotic.back.cpp import translator
from chaotic.front import parser as front_parser
from chaotic.front import ref_resolver
from chaotic.front import types


@dataclasses.dataclass(init=False)
class NameMapItem:
    pattern: re.Pattern
    dest: str  # string for str.format()

    def __init__(self, data: str):
        groups = data.split('=')
        if len(groups) != 2:
            raise Exception(f'-n arg must contain "=" ({data})')

        pattern, dest = groups
        self.pattern = re.compile(pattern)
        self.dest = dest

    def match(self, data: str) -> Optional[str]:
        match = self.pattern.fullmatch(data)  # pylint: disable=no-member
        if match:
            return self.dest.format(*match.groups())
        return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-n',
        '--name-map',
        type=NameMapItem,
        required=True,
        action='append',
        help='in-file path (e.g. /schemas/Type) to C++ type mapping',
    )

    parser.add_argument(
        '-f',
        '--file-map',
        type=NameMapItem,
        required=True,
        action='append',
        help='full filepath to virtual filepath mapping',
    )

    parser.add_argument(
        '-u',
        '--userver',
        type=str,
        default='userver',
        help='userver namespace',
    )

    parser.add_argument(
        '-e',
        '--erase-path-prefix',
        type=str,
        default='',
        help='Erase path prefix from $refs',
    )

    parser.add_argument(
        '--parse-extra-formats',
        action='store_true',
        help='Parse extra formats besides json (yaml, yaml_config)',
    )
    parser.add_argument(
        '--generate-serializers',
        action='store_true',
        help='Generate JSON serializers for generated types',
    )

    parser.add_argument(
        '-o',
        '--output-dir',
        type=str,
        required=True,
        help='Directory to generate files in',
    )
    parser.add_argument(
        '--relative-to',
        type=str,
        required=True,
        help='Directory for input files root',  # TODO: wording
    )
    parser.add_argument(
        '-I',
        '--include-dir',
        action='append',
        help='Path to search for include files for x-usrv-cpp-type',
    )
    parser.add_argument(
        '--clang-format',
        default='clang-format',
        help=(
            'clang-format binary name. Can be either binary name in $PATH or '
            'full filepath to a binary file. Set to empty for no formatting.'
        ),
    )
    parser.add_argument(
        'file', type=str, nargs='+', help='yaml/json input filename',
    )
    return parser.parse_args()


def generate_cpp_name_func(
        name_map: List[NameMapItem], erase_prefix: str,
) -> Callable:
    def cpp_name_func(schema_name: str) -> str:
        for item in name_map:
            s = erase_prefix + schema_name + '/'
            # print(f'x: {schema_name} {s}')
            cpp_name = item.match(s)
            if cpp_name:
                return cpp_name
        raise Exception(f'Cannot match name: {schema_name}')

    return cpp_name_func


def vfilepath_from_filepath(filepath: str, file_map: List[NameMapItem]) -> str:
    for item in file_map:
        vfilepath = item.match(filepath)
        if vfilepath:
            return vfilepath
    raise Exception(f'Cannot match path: {filepath}')


def traverse_dfs(path: str, data: Any):
    if not isinstance(data, dict):
        return

    items_to_remove = []
    for name, item in data.items():
        subpath = f'{path}{name}/'

        feed = None
        sticky_feed = False
        res = None
        gen = traverse_dfs(subpath, item)
        try:
            while True:
                res = gen.send(feed)
                feed = yield res
                sticky_feed = sticky_feed or feed
        except (GeneratorExit, StopIteration):
            if feed:
                items_to_remove.append(name)
            if sticky_feed and item == {}:
                items_to_remove.append(name)

    for item in items_to_remove:
        del data[item]

    yield path, data


def extract_schemas_to_scan(
        inp: dict, name_map: List[NameMapItem],
) -> Dict[str, Any]:
    schemas = []

    gen = traverse_dfs('/', inp)
    ok_ = None
    while True:
        try:
            path, data = gen.send(ok_)
            # print('path ', path)
            ok_ = False
            for item in name_map:
                if item.pattern.fullmatch(path):
                    schemas.append((path, data))
                    ok_ = True
                    break
        except (GeneratorExit, StopIteration):
            break

    return dict(schemas)


def read_schemas(
        erase_path_prefix: str,
        filepaths: List[str],
        name_map,
        file_map,
        dependencies: List[types.ResolvedSchemas] = [],
) -> types.ResolvedSchemas:
    config = front_parser.ParserConfig(erase_prefix=erase_path_prefix)
    rr = ref_resolver.RefResolver()

    schemas = []
    for fname in filepaths:
        with open(fname) as ifile:
            data = yaml.load(ifile, Loader=yaml.CLoader)

        scan_objects = extract_schemas_to_scan(data, name_map)

        vfilepath = vfilepath_from_filepath(fname, file_map)
        parser = front_parser.SchemaParser(
            config=config, full_filepath=fname, full_vfilepath=vfilepath,
        )
        for path, obj in rr.sort_json_types(
                scan_objects, erase_path_prefix,
        ).items():
            parser.parse_schema(path.rstrip('/'), obj)
        schemas.append(parser.parsed_schemas())

    dep_resolved = {}
    for dependency in dependencies:
        dep_resolved.update(dependency.schemas)
    return rr.sort_schemas(
        types.ParsedSchemas.merge(schemas),
        external_schemas=types.ResolvedSchemas(schemas=dep_resolved),
    )


def write_file(filepath: str, content: str) -> None:
    if os.path.exists(filepath):
        with open(filepath, 'r', encoding='utf8') as ifile:
            old_content = ifile.read()
            if old_content == content:
                return

    with open(filepath, 'w') as ofile:
        ofile.write(content)


def main() -> None:
    args = parse_args()

    schemas = read_schemas(
        args.erase_path_prefix, args.file, args.name_map, args.file_map,
    )
    cpp_name_func = generate_cpp_name_func(
        args.name_map, args.erase_path_prefix,
    )

    gen = translator.Generator(
        config=translator.GeneratorConfig(
            include_dirs=args.include_dir or [],
            namespaces={file: '' for file in args.file},
            infile_to_name_func=cpp_name_func,
        ),
    )
    types = gen.generate_types(schemas, external_schemas={})

    outputs = renderer.OneToOneFileRenderer(
        relative_to=args.relative_to,
        vfilepath_to_relfilepath={
            file: file.split('.')[0] for file in args.file
        },
        clang_format_bin=args.clang_format,
        parse_extra_formats=args.parse_extra_formats,
        generate_serializer=args.generate_serializers,
    ).render(types)
    for output in outputs:
        if output.filepath_wo_ext.startswith('/'):
            filename_rel = os.path.relpath(
                output.filepath_wo_ext, args.relative_to,
            )
        else:
            filename_rel = output.filepath_wo_ext

        dirname = os.path.dirname(os.path.join(args.output_dir, filename_rel))
        os.makedirs(dirname, exist_ok=True)

        for file in output.files:
            write_file(
                os.path.join(args.output_dir, filename_rel + file.ext),
                file.content,
            )
    print(
        f'Handled {len(types)} schema types in {len(outputs)} file groups.',
        file=sys.stderr,
    )


if __name__ == '__main__':
    main()
