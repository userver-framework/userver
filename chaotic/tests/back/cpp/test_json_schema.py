# Test that chaotic.back.cpp types contain the `json_schema` with chaotic.front.types

import pytest

from chaotic.back.cpp import translator
from chaotic.front import types


@pytest.fixture
def raw_gen(simple_parse, cpp_name_func):
    "Unlike `simple_gen` the `raw_gen` does not call `clean` and keeps the `.json_schema` fileds"

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


def test_boolean(raw_gen):
    result = raw_gen({'type': 'boolean'})

    assert result['::type'].json_schema == types.Boolean(
        x_properties={},
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        type='boolean',
    )


def test_number(raw_gen):
    result = raw_gen({'type': 'number', 'minimum': 1, 'maximum': 2.2})

    assert result['::type'].json_schema == types.Number(
        x_properties={},
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        type='number',
        minimum=1,
        maximum=2.2,
    )


def test_string(raw_gen):
    result = raw_gen({'type': 'string', 'format': 'uuid'})

    assert result['::type'].json_schema == types.String(
        x_properties={},
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        type='string',
        format=types.StringFormat.UUID,
    )


def test_array(raw_gen):
    result = raw_gen({
        'type': 'array',
        'minItems': 1,
        'maxItems': 3,
        'items': {'type': 'integer'},
    })

    assert result['::type'].json_schema == types.Array(
        x_properties={},
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        type='array',
        minItems=1,
        maxItems=3,
        items=types.Integer(
            x_properties={},
            source_location_=types.SourceLocation(
                filepath='vfull',
                location='/definitions/type/items',
            ),
            type='integer',
        ),
    )


def test_any_value(raw_gen):
    result = raw_gen({'description': 'any json value'})

    assert result['::type'].json_schema == types.AnyValue(
        x_properties={},
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        description='any json value',
    )


def test_const(raw_gen):
    result = raw_gen({'type': 'string', 'const': 'active'})

    assert result['::type'].json_schema == types.ConstSchema(
        x_properties={},
        source_location_=types.SourceLocation(
            filepath='vfull',
            location='/definitions/type',
        ),
        const='active',
        const_type=types.ConstType.STRING,
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
