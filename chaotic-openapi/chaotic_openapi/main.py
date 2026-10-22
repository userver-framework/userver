import argparse
import dataclasses
import os
import pathlib
import posixpath
import sys
from typing import NoReturn

import yaml

from chaotic import error as chaotic_error
from chaotic import layout
from chaotic_openapi.back.cpp.client import renderer as client_renderer
from chaotic_openapi.back.cpp.client import translator as client_translator
from chaotic_openapi.back.cpp.handler import renderer as handler_renderer
from chaotic_openapi.back.cpp.handler import translator as handler_translator
from chaotic_openapi.front import parser as front_parser
from chaotic_openapi.front import ref_resolver


def die(msg: str) -> NoReturn:
    print(msg, file=sys.stderr)
    sys.exit(1)


def main():
    try:
        do_main()
    except chaotic_error.BaseError as exc:
        die(str(exc))


def load_documents(files: list[str]) -> tuple[dict, layout.Closure | None]:
    inputs = {}
    for file in map(os.path.normpath, files):
        with open(file) as ifile:
            inputs[file] = yaml.safe_load(ifile)
    closure = layout.load_closure(inputs)
    return (closure.documents if closure else inputs), closure


def with_role(closure: layout.Closure, role: str) -> tuple[layout.Closure, str, str]:
    """
    Moves the documents declaring operations into the role's sub-namespace and directory: a client
    and handlers generated from one document are different code and must not share names or files.

    Returns the closure and the namespace and the directory of the spec's own files.
    """
    with_operations = [path for path in closure.inputs if closure.documents[path].get('paths')]
    for path in with_operations:
        document = closure.documents[path]
        if (document.get('components') or {}).get('schemas') or document.get('definitions'):
            die(
                f'{path}: a document with operations and {layout.NAMESPACE_TAG} cannot declare schemas, '
                'move them to a schema document'
            )
    layouts = {closure.layouts[path] for path in with_operations or closure.inputs[:1]}
    if len({(document_layout.namespace, document_layout.directory) for document_layout in layouts}) > 1:
        die(
            'Documents with operations must share their namespace and directory: '
            + ', '.join(sorted(f'{path} ({closure.layouts[path].namespace})' for path in with_operations))
        )
    spec_layout = next(iter(layouts))
    role_namespace = f'{spec_layout.namespace}::{role}'
    role_directory = posixpath.join(spec_layout.directory, role)

    layouts_with_role = dict(closure.layouts)
    for path in with_operations:
        layouts_with_role[path] = layout.DocumentLayout(
            namespace=role_namespace,
            path=posixpath.join(role_directory, posixpath.basename(closure.layouts[path].path)),
        )
    return dataclasses.replace(closure, layouts=layouts_with_role), role_namespace, role_directory


def do_main():
    args = parse_args()

    contents, closure = load_documents(args.files)
    if closure and args.namespace:
        die(f'--namespace is forbidden for documents with {layout.NAMESPACE_TAG}')
    if args.list_dependencies:
        print(';'.join(closure.dependencies if closure else []))
        return
    sorted_contents = ref_resolver.sort_openapis(contents)

    # parse
    parser = front_parser.Parser(args.name)
    for file, content in sorted_contents:
        parser.parse_schema(content, file, file, with_operations=closure is None or file in closure.inputs)

    if closure:
        closure, cpp_namespace, include_dir = with_role(closure, 'client' if args.gen == 'client' else 'handlers')
    else:
        cpp_namespace, include_dir = None, None
    clang_format = '' if args.list_outputs else args.clang_format

    ctx = client_renderer.Context(
        generate_path='',
        clang_format_bin=clang_format,
        uservices_library_tvm_guard_hack=False,
    )

    outputs: list[client_renderer.CppOutput] = []
    views: list[client_renderer.CppOutput] = []
    config_content = None
    if args.gen == 'client':
        if not args.output_dir:
            die('-o is required for --gen client')
        spec = client_translator.Translator(
            parser.service(),
            dynamic_config=args.dynamic_config,
            cpp_namespace=(cpp_namespace or args.namespace or f'clients::{args.name}'),
            include_dirs=None if not args.check_includes else args.include_dirs or [],
            middleware_plugins=[],
            closure=closure,
            include_dir=include_dir,
        ).spec()
        outputs = client_renderer.render(spec, ctx, schema_files=[os.path.basename(f) for f in args.files])

    elif args.gen in ('handlers', 'handlers+views', 'views'):
        if args.gen != 'views' and not args.output_dir:
            die(f'-o is required for --gen {args.gen}')
        if args.gen in ('handlers+views', 'views') and not args.src_dir:
            die(f'--src-dir is required for --gen {args.gen}')

        spec = handler_translator.HandlerTranslator(
            parser.service(),
            cpp_namespace=(cpp_namespace or args.namespace or f'handlers::{args.name}'),
            include_dirs=None if not args.check_includes else args.include_dirs or [],
            closure=closure,
            include_dir=include_dir,
        ).spec()

        if args.gen in ('handlers', 'handlers+views'):
            outputs = handler_renderer.render(spec, ctx, args.userver)
            config_content = handler_renderer.render_config_yaml(spec)
        if args.gen in ('handlers+views', 'views'):
            views = handler_renderer.render_views(spec, ctx, args.userver)

    else:
        die(f'Unknown --gen value: {args.gen!r}')

    if args.list_outputs:
        files = [os.path.join(args.output_dir, output.rel_path) for output in outputs]
        if config_content is not None:
            files.append(os.path.join(args.output_dir, handler_renderer.config_yaml_path(spec)))
        files += [os.path.join(args.src_dir, view.rel_path) for view in views]
        print(';'.join(files))
        return

    if outputs:
        client_renderer.CppOutput.save(outputs, args.output_dir)
    if args.gen == 'client' and not args.dynamic_config:
        qos_header = pathlib.Path(args.output_dir) / 'include' / spec.include_dir / 'qos.hpp'
        qos_header.unlink(missing_ok=True)
    if config_content is not None:
        handler_renderer.save_config_yaml(config_content, args.output_dir, handler_renderer.config_yaml_path(spec))
    if views:
        handler_renderer.save_views(views, args.src_dir)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', required=True, help='Client/Service name')
    parser.add_argument('-o', '--output-dir', required=False, default=None)
    parser.add_argument('--namespace', required=False)
    parser.add_argument(
        '-u',
        '--userver',
        default='USERVER_NAMESPACE',
        help='userver namespace macro name',
    )
    parser.add_argument(
        '--src-dir',
        required=False,
        default=None,
        help='Source root for generated view stubs (--gen views or handlers+views)',
    )
    parser.add_argument(
        '--gen',
        choices=['client', 'handlers', 'views', 'handlers+views'],
        help=('What to generate: client code, handler base classes, view stubs, or both handlers and view stubs'),
    )
    parser.add_argument('--dynamic-config', default='')
    parser.add_argument(
        '--list-outputs',
        action='store_true',
        help='Print the files to generate, view stubs included, semicolon-separated, and exit without generating',
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
        '--clang-format',
        default='clang-format',
        help=(
            'clang-format binary name. Can be either binary name in $PATH or '
            'full filepath to a binary file. Set to empty for no formatting.'
        ),
    )
    parser.add_argument(
        '-I',
        '--include-dir',
        dest='include_dirs',
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
        'files',
        nargs='+',
        help='openapi/swagger yaml/json schemas',
    )
    return parser.parse_args()


if __name__ == '__main__':
    main()
