from chaotic.back.cpp import type_name
from chaotic.back.cpp.translator import Generator
from chaotic.back.cpp.translator import GeneratorConfig
from chaotic.back.cpp.types import CppRef
from chaotic.back.cpp.types import CppStruct
from chaotic.front import ref_resolver
from chaotic.front.parser import ParserConfig
from chaotic.front.parser import SchemaParser
from chaotic.front.types import SchemaObject
from chaotic.main import generate_cpp_name_func
from chaotic.main import NameMapItem


def test_simple_ref(clean):
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )

    parser.parse_schema(
        '/definitions/Type',
        {'type': 'object', 'properties': {}, 'additionalProperties': False},
    )
    parser.parse_schema('/definitions/ref', {'$ref': '#/definitions/Type'})

    cpp_name_func = generate_cpp_name_func(
        [NameMapItem('/definitions/([^/]*)/={0}')], '',
    )

    schemas = parser.parsed_schemas()
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(schemas)
    gen = Generator(
        config=GeneratorConfig(
            namespaces={'vfull': ''}, infile_to_name_func=cpp_name_func,
        ),
    )
    cpp_types = gen.generate_types(resolved_schemas)
    cpp_types = clean(cpp_types)

    assert cpp_types == {
        'Type': CppStruct(
            raw_cpp_type=type_name.TypeName('Type'),
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            fields={},
        ),
        'ref': CppRef(
            raw_cpp_type=type_name.TypeName(''),
            json_schema=None,
            nullable=False,
            indirect=False,
            self_ref=False,
            user_cpp_type=None,
            orig_cpp_type=CppStruct(
                raw_cpp_type=type_name.TypeName('Type'),
                json_schema=SchemaObject(
                    additionalProperties=False, properties={},
                ),
                nullable=False,
                user_cpp_type=None,
                fields={},
            ),
        ),
    }
