import pytest

from chaotic import main
from chaotic.back.cpp import translator
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import ref_resolver
from chaotic.front import types as front_types


@pytest.mark.parametrize('additional_property1, additional_property2', [(False, False), (True, False), (True, True)])
def test_two_element_allof_schema(clean, schema_parser, additional_property1, additional_property2):
    parser = schema_parser

    parser.parse_schema(
        '/definitions/A',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'string'}},
            'additionalProperties': additional_property1,
        },
    )

    parser.parse_schema(
        '/definitions/B',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'string'}},
            'additionalProperties': additional_property2,
        },
    )

    parser.parse_schema(
        '/definitions/allOf',
        {
            'type': 'object',
            'properties': {
                'foo': {
                    'allOf': [{'$ref': '#/definitions/A'}, {'$ref': '#/definitions/B'}],
                },
            },
            'additionalProperties': False,
        },
    )

    cpp_name_func = main.generate_cpp_name_func(
        [main.NameMapItem('/definitions/([^/]*)/={0}')],
        '',
    )

    schemas = parser.parsed_schemas()
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(schemas)
    gen = translator.Generator(
        config=translator.GeneratorConfig(
            namespaces={'vfull': ''},
            infile_to_name_func=cpp_name_func,
        ),
    )
    generated_cpp_types = gen.generate_types(resolved_schemas)
    generated_cpp_types = clean(generated_cpp_types)

    foo_schema = generated_cpp_types['::allOf'].fields['foo'].schema

    assert foo_schema.parents == [
        cpp_types.CppRef(
            raw_cpp_type=type_name.TypeName('::allOf::Foo__P0'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            orig_cpp_type=cpp_types.CppStruct(
                raw_cpp_type=type_name.TypeName('::A'),
                json_schema=front_types.Schema(),
                nullable=False,
                user_cpp_type=None,
                fields={
                    'bar': cpp_types.CppStructField(
                        name='bar',
                        required=False,
                        schema=cpp_types.CppPrimitiveType(
                            raw_cpp_type=type_name.TypeName('std::string'),
                            json_schema=front_types.Schema(),
                            nullable=False,
                            user_cpp_type=None,
                            validators=cpp_types.CppPrimitiveValidator(
                                namespace='::A',
                                prefix='Bar',
                            ),
                        ),
                    )
                },
                extra_type=True if additional_property1 else None,
                autodiscover_default_dict=False,
                strict_parsing=True,
            ),
            indirect=False,
            self_ref=False,
            cpp_name='::A',
        ),
        cpp_types.CppRef(
            raw_cpp_type=type_name.TypeName('::allOf::Foo__P1'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            orig_cpp_type=cpp_types.CppStruct(
                raw_cpp_type=type_name.TypeName('::B'),
                json_schema=front_types.Schema(),
                nullable=False,
                user_cpp_type=None,
                fields={
                    'bar': cpp_types.CppStructField(
                        name='bar',
                        required=False,
                        schema=cpp_types.CppPrimitiveType(
                            raw_cpp_type=type_name.TypeName('std::string'),
                            json_schema=front_types.Schema(),
                            nullable=False,
                            user_cpp_type=None,
                            default=None,
                            validators=cpp_types.CppPrimitiveValidator(
                                namespace='::B',
                                prefix='Bar',
                            ),
                        ),
                    )
                },
                extra_type=True if additional_property2 else None,
                autodiscover_default_dict=False,
                strict_parsing=True,
            ),
            indirect=False,
            self_ref=False,
            cpp_name='::B',
        ),
    ], f'Actual parents: {foo_schema.parents}'
