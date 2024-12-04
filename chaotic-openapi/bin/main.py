from chaotic_openapi.back.cpp_client import renderer
from chaotic_openapi.back.cpp_client import types

from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types


def main():
    ctx = renderer.Context(generate_path='', clang_format_bin='')
    spec = types.ClientSpec(
        client_name='test',
        cpp_namespace='clients::test',
        operations=[
            types.Operation(
                method='POST',
                path='testme',
                description='A testing method to call',
                parameters=[
                    types.Parameter(
                        raw_name='number',
                        cpp_name='number',
                        parser='TrivialParameter',
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
            ),
        ],
    )
    outputs = renderer.render(spec, ctx)
    for output in outputs:
        print(f'\n{output.rel_path}\n====\n{output.content}')


if __name__ == '__main__':
    main()
