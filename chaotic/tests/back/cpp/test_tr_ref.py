from chaotic.back.cpp import translator
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import ref_resolver
from chaotic.front import types as front_types


def test_simple_ref(clean, cpp_name_func, schema_parser):
    parser = schema_parser

    parser.parse_schema(
        '/definitions/Type',
        {'type': 'object', 'properties': {}, 'additionalProperties': False},
    )
    parser.parse_schema('/definitions/ref', {'$ref': '#/definitions/Type'})

    schemas = parser.parsed_schemas()
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(schemas)
    gen = translator.Generator(
        config=translator.GeneratorConfig(
            namespaces={'vfull': ''},
            infile_to_name_func=cpp_name_func,
        ),
    )
    result = gen.generate_types(resolved_schemas)
    result = clean(result)

    assert result == {
        '::Type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::Type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            fields={},
        ),
        '::ref': cpp_types.CppRef(
            raw_cpp_type=type_name.TypeName('::ref'),
            json_schema=front_types.Schema(),
            nullable=False,
            indirect=False,
            self_ref=False,
            cpp_name='::Type',
            user_cpp_type=None,
            orig_cpp_type=cpp_types.CppStruct(
                raw_cpp_type=type_name.TypeName('::Type'),
                json_schema=front_types.Schema(),
                nullable=False,
                user_cpp_type=None,
                fields={},
            ),
        ),
    }
