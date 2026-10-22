import argparse
from collections.abc import Callable
import dataclasses
import os
import pathlib
import re
import sys
from typing import Any
from typing import NoReturn

import yaml

from chaotic import error
from chaotic import layout
from chaotic.back.cpp import renderer
from chaotic.back.cpp import tagged
from chaotic.back.cpp import translator
from chaotic.back.cpp import types as cpp_types
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

    def match(self, data: str, *, stem: str) -> str | None:
        match = self.pattern.fullmatch(data)  # pylint: disable=no-member
        if match:
            return self.dest.format(*match.groups(), stem=stem)
        return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-n',
        '--name-map',
        type=NameMapItem,
        action='append',
        help=(
            'in-file path (e.g. /schemas/Type) to C++ type mapping, '
            'required for documents without x-usrv-cpp-namespace and forbidden for tagged ones'
        ),
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
        '--no-sax-parse',
        action='store_false',
        dest='generate_sax_parser',
        help='Do not generate JSON SAX parsers',
    )
    parser.add_argument(
        '--no-stream-writer',
        action='store_false',
        dest='generate_stream_writer',
        help='Do not generate JSON stream writers (serializers)',
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
        help=(
            'Directory for input files root, '
            'required for documents without x-usrv-cpp-namespace and forbidden for tagged ones'
        ),
    )
    parser.add_argument(
        '--list-outputs',
        action='store_true',
        help=('Print the files to generate, semicolon-separated, and exit without generating'),
    )
    parser.add_argument(
        '--list-dependencies',
        action='store_true',
        help=(
            'Print the documents generated elsewhere that the inputs refer to, semicolon-separated, '
            'and exit without generating'
        ),
    )
    parser.add_argument(
        '-I',
        '--include-dir',
        action='append',
        help=(
            'Path to search for include files for x-usrv-cpp-type, used only '
            'to produce a nicer error message if the header is missing.'
        ),
    )
    parser.add_argument(
        '--no-check-includes',
        action='store_false',
        dest='check_includes',
        help='Do not check that x-usrv-cpp-type headers exist',
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
        'file',
        type=str,
        nargs='+',
        help='yaml/json input filename',
    )
    return parser.parse_args()


def generate_cpp_name_func(
    name_map: list[NameMapItem],
    erase_prefix: str,
) -> Callable:
    def cpp_name_func(schema_name: str, stem: str) -> str:
        for item in name_map:
            s = erase_prefix + schema_name + '/'
            cpp_name = item.match(s, stem=stem)
            if cpp_name:
                return cpp_name
        raise Exception(f'Cannot match name: {schema_name}')

    return cpp_name_func


def vfilepath_from_filepath(filepath: str, file_map: list[NameMapItem]) -> str:
    for item in file_map:
        vfilepath = item.match(filepath, stem=pathlib.Path(filepath).stem)
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
            elif sticky_feed and item == {}:
                items_to_remove.append(name)

    for item in items_to_remove:
        del data[item]

    yield path, data


def extract_schemas_to_scan(
    inp: dict,
    name_map: list[NameMapItem],
) -> dict[str, Any]:
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
    filepaths: list[str],
    name_map,
    file_map,
    dependencies: list[types.ResolvedSchemas] = [],
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
            config=config,
            full_filepath=fname,
            full_vfilepath=vfilepath,
        )
        for path, obj in rr.sort_json_types(
            scan_objects,
            erase_path_prefix,
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


def read_tagged_schemas(
    closure: layout.Closure,
    include_dirs: list[str] | None,
) -> dict[str, cpp_types.CppType]:
    rr = ref_resolver.RefResolver()
    type_roots = [NameMapItem(layout.TYPE_ROOTS_RE.pattern + '={0}')]

    def parse(paths: list[str]) -> types.ParsedSchemas:
        schemas = []
        for path in paths:
            parser = front_parser.SchemaParser(
                config=front_parser.ParserConfig(erase_prefix=''),
                full_filepath=path,
                full_vfilepath=path,
            )
            scan_objects = extract_schemas_to_scan(closure.documents[path], type_roots)
            for infile_path, obj in rr.sort_json_types(scan_objects).items():
                parser.parse_schema(infile_path.rstrip('/'), obj)
            schemas.append(parser.parsed_schemas())
        return types.ParsedSchemas.merge(schemas)

    def make_generator(include_dirs: list[str] | None) -> translator.Generator:
        def name_func(infile_path: str, stem: str) -> str:
            name = layout.type_name(infile_path)
            assert name, infile_path
            return name

        return translator.Generator(
            config=translator.GeneratorConfig(
                include_dirs=include_dirs,
                namespaces={path: document_layout.namespace for path, document_layout in closure.layouts.items()},
                infile_to_name_func=name_func,
            ),
        )

    # Dependencies are generated elsewhere, their types are only referred to
    dependency_schemas = rr.sort_schemas(parse(closure.dependencies))
    dependency_types = make_generator(None).generate_types(dependency_schemas)
    input_schemas = rr.sort_schemas(parse(closure.inputs), external_schemas=dependency_schemas)
    return make_generator(include_dirs).generate_types(input_schemas, external_schemas=dependency_types)


def main_tagged(args: argparse.Namespace, closure: layout.Closure) -> None:
    for name in ('name_map', 'relative_to'):
        if getattr(args, name):
            option = '--' + name.replace('_', '-')
            die(f'{option} is forbidden for documents with {layout.NAMESPACE_TAG}')
    if args.erase_path_prefix:
        die(f'--erase-path-prefix is forbidden for documents with {layout.NAMESPACE_TAG}')
    # Every generator renders a tagged document alike, see chaotic.layout
    if (
        args.generate_serializers
        or args.parse_extra_formats
        or not args.generate_sax_parser
        or not args.generate_stream_writer
    ):
        die(
            '--generate-serializers, --parse-extra-formats, --no-sax-parse and --no-stream-writer '
            f'are forbidden for documents with {layout.NAMESPACE_TAG}'
        )
    for path in closure.inputs:
        sections = layout.openapi_only_sections(closure.documents[path])
        if sections:
            die(f'{path}: {", ".join(sections)} can only be generated by chaotic-openapi-gen')

    if args.list_outputs:
        outputs = [
            os.path.join(args.output_dir, file)
            for path in closure.inputs
            for file in layout.output_files(closure.layouts[path])
        ]
        print(';'.join(outputs))
        return
    if args.list_dependencies:
        print(';'.join(closure.dependencies))
        return

    try:
        types = read_tagged_schemas(closure, None if not args.check_includes else args.include_dir or [])
    except error.BaseError as exc:
        die(str(exc))

    outputs = tagged.render(types, closure, args.clang_format)
    for output in outputs:
        for file in output.files:
            filepath = os.path.join(args.output_dir, file.subdir, output.filepath_wo_ext + file.ext)
            os.makedirs(os.path.dirname(filepath), exist_ok=True)
            write_file(filepath, file.content)
    print(
        f'Handled {len(types)} schema types in {len(outputs)} file groups.',
        file=sys.stderr,
    )


def untagged_output_files(args: argparse.Namespace) -> list[str]:
    """The files main() writes for documents without x-usrv-cpp-namespace, inferred without parsing."""
    extensions = ['.cpp', '.hpp', '_fwd.hpp', '_parsers.ipp']
    if args.generate_sax_parser:
        extensions.append('_sax_parsers.hpp')
    files = []
    for file in args.file:
        # Mirrors the output paths of OneToOneFileRenderer below
        filepath_wo_ext = renderer.OneToOneFileRenderer.filepath_wo_ext(str(pathlib.Path(file).with_suffix('')))
        if filepath_wo_ext.startswith('/'):
            filepath_wo_ext = os.path.relpath(filepath_wo_ext, args.relative_to)
        files.extend(os.path.join(args.output_dir, filepath_wo_ext + extension) for extension in extensions)
    return files


def die(msg: str) -> NoReturn:
    print(msg, file=sys.stderr)
    sys.exit(1)


def main() -> None:
    args = parse_args()

    inputs = {}
    for file in map(os.path.normpath, args.file):
        with open(file) as ifile:
            inputs[file] = yaml.load(ifile, Loader=yaml.CLoader)
    try:
        closure = layout.load_closure(inputs)
    except error.BaseError as exc:
        die(str(exc))
    if closure is not None:
        main_tagged(args, closure)
        return
    if not args.name_map or not args.relative_to:
        die(f'--name-map and --relative-to are required for documents without {layout.NAMESPACE_TAG}')
    if args.list_outputs:
        print(';'.join(untagged_output_files(args)))
        return
    if args.list_dependencies:
        # Documents without x-usrv-cpp-namespace refer only to the inputs
        print('')
        return

    try:
        schemas = read_schemas(
            args.erase_path_prefix,
            args.file,
            args.name_map,
            [NameMapItem('(.*)={0}')],
        )
        cpp_name_func = generate_cpp_name_func(
            args.name_map,
            args.erase_path_prefix,
        )

        gen = translator.Generator(
            config=translator.GeneratorConfig(
                include_dirs=None if not args.check_includes else args.include_dir or [],
                namespaces={file: '' for file in args.file},
                infile_to_name_func=cpp_name_func,
            ),
        )
        types = gen.generate_types(schemas, external_schemas={})
    except error.BaseError as exc:
        print(exc, file=sys.stderr)
        sys.exit(1)

    outputs = renderer.OneToOneFileRenderer(
        relative_to=args.relative_to,
        vfilepath_to_relfilepath={file: str(pathlib.Path(file).with_suffix('')) for file in args.file},
        clang_format_bin=args.clang_format,
        parse_extra_formats=args.parse_extra_formats,
        generate_serializer=args.generate_serializers,
        generate_sax_parser=args.generate_sax_parser,
        generate_stream_writer=args.generate_stream_writer,
    ).render(types)
    for output in outputs:
        if output.filepath_wo_ext.startswith('/'):
            filename_rel = os.path.relpath(
                output.filepath_wo_ext,
                args.relative_to,
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
