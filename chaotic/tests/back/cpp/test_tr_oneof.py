from chaotic.back.cpp import type_name
from chaotic.back.cpp.translator import Generator
from chaotic.back.cpp.translator import GeneratorConfig
from chaotic.front import ref_resolver
from chaotic.front.parser import ParserConfig
from chaotic.front.parser import SchemaParser
from chaotic.front.types import MappingType
from chaotic.main import generate_cpp_name_func
from chaotic.main import NameMapItem


def test_simple(simple_gen):
    foo_schema = (
        simple_gen({
            'type': 'object',
            'properties': {
                'foo': {
                    'oneOf': [
                        {'type': 'integer'},
                        {'type': 'string'},
                        {'type': 'number'},
                    ]
                },
            },
            'additionalProperties': False,
        })['::type']
        .fields['foo']
        .schema
    )

    assert foo_schema.raw_cpp_type == type_name.TypeName(['', 'type', 'Foo'])
    assert len(foo_schema.variants) == 3

    assert foo_schema.variants[0].raw_cpp_type == type_name.TypeName('int')
    assert foo_schema.variants[1].raw_cpp_type == type_name.TypeName(['std', 'string'])
    assert foo_schema.variants[2].raw_cpp_type == type_name.TypeName('double')


def test_empty_mapping(clean):
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )

    parser.parse_schema(
        '/definitions/A',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'string'}},
            'additionalProperties': False,
        },
    )

    parser.parse_schema(
        '/definitions/B',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'string'}},
            'additionalProperties': False,
        },
    )

    parser.parse_schema(
        '/definitions/oneof',
        {
            'type': 'object',
            'properties': {
                'foo': {
                    'oneOf': [{'$ref': '#/definitions/A'}, {'$ref': '#/definitions/B'}],
                    'discriminator': {
                        'propertyName': 'bar',
                    },
                },
            },
            'additionalProperties': False,
        },
    )

    cpp_name_func = generate_cpp_name_func(
        [NameMapItem('/definitions/([^/]*)/={0}')],
        '',
    )

    schemas = parser.parsed_schemas()
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(schemas)
    gen = Generator(
        config=GeneratorConfig(
            namespaces={'vfull': ''},
            infile_to_name_func=cpp_name_func,
        ),
    )
    cpp_types = gen.generate_types(resolved_schemas)
    cpp_types = clean(cpp_types)

    foo = cpp_types['::oneof'].fields['foo'].schema

    assert foo.mapping_type == MappingType.STR
    assert list(foo.variants.keys()) == ['A', 'B']


def test_str_mapping(clean):
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )

    parser.parse_schema(
        '/definitions/A',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'string'}},
            'additionalProperties': False,
        },
    )

    parser.parse_schema(
        '/definitions/B',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'string'}},
            'additionalProperties': False,
        },
    )

    parser.parse_schema(
        '/definitions/oneof',
        {
            'type': 'object',
            'properties': {
                'foo': {
                    'oneOf': [{'$ref': '#/definitions/A'}, {'$ref': '#/definitions/B'}],
                    'discriminator': {
                        'propertyName': 'bar',
                        'mapping': {'aaa': '#/definitions/A', 'bbb': '#/definitions/B'},
                    },
                },
            },
            'additionalProperties': False,
        },
    )

    cpp_name_func = generate_cpp_name_func(
        [NameMapItem('/definitions/([^/]*)/={0}')],
        '',
    )

    schemas = parser.parsed_schemas()
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(schemas)
    gen = Generator(
        config=GeneratorConfig(
            namespaces={'vfull': ''},
            infile_to_name_func=cpp_name_func,
        ),
    )
    cpp_types = gen.generate_types(resolved_schemas)
    cpp_types = clean(cpp_types)

    foo = cpp_types['::oneof'].fields['foo'].schema

    assert foo.mapping_type == MappingType.STR
    assert list(foo.variants.keys()) == ['aaa', 'bbb']

    assert foo.variants['aaa'].cpp_name == '::A'
    assert foo.variants['bbb'].cpp_name == '::B'


def test_int_mapping(clean):
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )

    parser.parse_schema(
        '/definitions/A',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'integer'}},
            'additionalProperties': False,
        },
    )

    parser.parse_schema(
        '/definitions/B',
        {
            'type': 'object',
            'properties': {'bar': {'type': 'integer'}},
            'additionalProperties': False,
        },
    )

    parser.parse_schema(
        '/definitions/oneof',
        {
            'type': 'object',
            'properties': {
                'foo': {
                    'oneOf': [{'$ref': '#/definitions/A'}, {'$ref': '#/definitions/B'}],
                    'discriminator': {
                        'propertyName': 'bar',
                        'mapping': {0: '#/definitions/A', 1: '#/definitions/A', 2: '#/definitions/B'},
                    },
                },
            },
            'additionalProperties': False,
        },
    )

    cpp_name_func = generate_cpp_name_func(
        [NameMapItem('/definitions/([^/]*)/={0}')],
        '',
    )

    schemas = parser.parsed_schemas()
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(schemas)
    gen = Generator(
        config=GeneratorConfig(
            namespaces={'vfull': ''},
            infile_to_name_func=cpp_name_func,
        ),
    )
    cpp_types = gen.generate_types(resolved_schemas)
    cpp_types = clean(cpp_types)

    foo = cpp_types['::oneof'].fields['foo'].schema

    assert foo.mapping_type == MappingType.INT
    assert list(foo.variants.keys()) == [0, 1, 2]

    assert foo.variants[0].cpp_name == '::A'
    assert foo.variants[1].cpp_name == '::A'
    assert foo.variants[2].cpp_name == '::B'
