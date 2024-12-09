import argparse

from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic_openapi.back.cpp_client import renderer
from chaotic_openapi.back.cpp_client import types


def main():
    args = parse_args()

    ctx = renderer.Context(
        generate_path='', clang_format_bin=args.clang_format,
    )
    spec = types.ClientSpec(
        client_name=args.name,
        cpp_namespace=f'clients::{args.name}',
        operations=[
            types.Operation(
                method='POST',
                path='/testme',
                description='A testing method to call',
                parameters=[
                    types.Parameter(
                        raw_name='number',
                        cpp_name='number',
                        cpp_type='int',
                        parser='openapi::TrivialParameter<openapi::In::kQuery, knumber, int>',
                    ),
                ],
                request_bodies=[
                    types.RequestBody(
                        content_type='text/plaintext',
                        schema=cpp_types.CppPrimitiveType(
                            raw_cpp_type=type_name.TypeName('int'),
                            user_cpp_type=None,
                            json_schema=None,
                            nullable=False,
                            validators=cpp_types.CppPrimitiveValidator(
                                prefix='/definitions/type',
                            ),
                        ),
                    ),
                ],
                responses=[],
            ),
        ],
    )
    outputs = renderer.render(spec, ctx)
    renderer.CppOutput.save(outputs, args.output_dir)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', required=True, help='Client name')
    parser.add_argument('-o', '--output-dir', required=True)
    parser.add_argument(
        '--clang-format',
        default='clang-format',
        help=(
            'clang-format binary name. Can be either binary name in $PATH or '
            'full filepath to a binary file. Set to empty for no formatting.'
        ),
    )
    parser.add_argument(
        'file', nargs='+', help='openapi/swagger yaml/json schemas',
    )
    return parser.parse_args()


if __name__ == '__main__':
    main()
