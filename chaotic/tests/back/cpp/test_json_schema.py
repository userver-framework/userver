import pytest

from chaotic.back.cpp import translator
from chaotic.front import types


@pytest.fixture
def raw_gen(simple_parse, cpp_name_func):
    def func(input_: dict):
        schemas = simple_parse(input_, clear=False)
        gen = translator.Generator(
            config=translator.GeneratorConfig(
                namespaces={'vfull': ''},
                include_dirs=None,
                infile_to_name_func=cpp_name_func,
            ),
        )
        return gen.generate_types(schemas)

    return func


def test_primitive(cpp_primitive_type, simple_parse, cpp_name_func, raw_gen):
    result = raw_gen({'type': 'integer'})

    assert result['::type'].json_schema == types.Integer(
        x_properties={},
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        type='integer',
    )


def test_object(cpp_primitive_type, simple_parse, cpp_name_func, raw_gen):
    result = raw_gen({
        'type': 'object',
        'properties': {},
        'additionalProperties': {'type': 'integer'},
    })
    assert result['::type'].json_schema == types.SchemaObject(
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        type='object',
        additionalProperties=types.Integer(
            source_location_=types.SourceLocation(
                filepath='vfull',
                location='/definitions/type/additionalProperties',
            ),
            type='integer',
        ),
        properties={},
    )
