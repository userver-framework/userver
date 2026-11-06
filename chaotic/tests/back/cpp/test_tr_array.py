import pytest

from chaotic.back.cpp import translator as cpp_translator
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types


def test_array_int(simple_gen, cpp_primitive_type):
    types = simple_gen({'type': 'array', 'items': {'type': 'integer'}})
    assert types == {
        '::type': cpp_types.CppArray(
            raw_cpp_type=type_name.TypeName('::type'),
            user_cpp_type=None,
            json_schema=front_types.Schema(),
            nullable=False,
            items=cpp_primitive_type(
                validators=cpp_types.CppPrimitiveValidator(prefix='typeA'),
                raw_cpp_type_str='int',
            ),
            container='std::vector',
            validators=cpp_types.CppArrayValidator(),
        ),
    }


def test_array_minmax_items(simple_gen, cpp_primitive_type):
    types = simple_gen({
        'type': 'array',
        'minItems': 1,
        'maxItems': 3,
        'items': {'type': 'integer'},
    })
    assert types == {
        '::type': cpp_types.CppArray(
            raw_cpp_type=type_name.TypeName('::type'),
            user_cpp_type=None,
            json_schema=front_types.Schema(),
            nullable=False,
            items=cpp_primitive_type(
                validators=cpp_types.CppPrimitiveValidator(prefix='typeA'),
                raw_cpp_type_str='int',
            ),
            container='std::vector',
            validators=cpp_types.CppArrayValidator(minItems=1, maxItems=3),
        ),
    }


@pytest.mark.parametrize(
    'item_schema,expected_cpp_type,expected_validators',
    [
        (
            {'type': 'integer'},
            'int',
            cpp_types.CppPrimitiveValidator(prefix='typeA'),
        ),
        (
            {'type': 'string'},
            'std::string',
            cpp_types.CppPrimitiveValidator(prefix='typeA'),
        ),
        (
            {'type': 'boolean'},
            'bool',
            cpp_types.CppPrimitiveValidator(),
        ),
    ],
)
def test_array_unique_items_valid_types(
    simple_gen,
    cpp_primitive_type,
    item_schema,
    expected_cpp_type,
    expected_validators,
):
    types = simple_gen({
        'type': 'array',
        'uniqueItems': True,
        'items': item_schema,
    })
    assert types == {
        '::type': cpp_types.CppArray(
            raw_cpp_type=type_name.TypeName('::type'),
            user_cpp_type=None,
            json_schema=front_types.Schema(),
            nullable=False,
            items=cpp_primitive_type(
                validators=expected_validators,
                raw_cpp_type_str=expected_cpp_type,
            ),
            container='std::vector',
            validators=cpp_types.CppArrayValidator(uniqueItems=True),
        ),
    }


@pytest.mark.parametrize(
    'item_schema',
    [
        {'type': 'number'},
        {'type': 'array', 'items': {'type': 'integer'}},
    ],
)
def test_array_unique_items_invalid_type(simple_gen, item_schema):
    with pytest.raises(cpp_translator.TranslatorError) as exc:
        simple_gen({
            'type': 'array',
            'uniqueItems': True,
            'items': item_schema,
        })
    assert exc.value.msg == ('uniqueItems is only supported for integer, string, and boolean item types')


def test_array_array_with_validators(simple_gen, cpp_primitive_type):
    types = simple_gen({
        'type': 'array',
        'items': {'type': 'array', 'items': {'type': 'integer', 'minimum': 1}},
    })
    assert types == {
        '::type': cpp_types.CppArray(
            raw_cpp_type=type_name.TypeName('::type'),
            user_cpp_type=None,
            json_schema=front_types.Schema(),
            nullable=False,
            items=cpp_types.CppArray(
                raw_cpp_type=type_name.TypeName('::typeA'),
                user_cpp_type=None,
                json_schema=front_types.Schema(),
                nullable=False,
                items=cpp_primitive_type(
                    validators=cpp_types.CppPrimitiveValidator(
                        min=1,
                        prefix='typeAA',
                    ),
                    raw_cpp_type_str='int',
                ),
                container='std::vector',
                validators=cpp_types.CppArrayValidator(),
            ),
            container='std::vector',
            validators=cpp_types.CppArrayValidator(),
        ),
    }


def _array_field(simple_gen, array_schema: dict) -> cpp_types.CppStructField:
    types = simple_gen({
        'type': 'object',
        'additionalProperties': False,
        'properties': {'field': array_schema},
    })
    return types['::type'].fields['field']


@pytest.mark.parametrize(
    'array_schema,field_type,initializer,parse_default',
    [
        (
            {'type': 'array', 'items': {'type': 'string'}},
            'std::optional<std::vector<std::string>>',
            '',
            '',
        ),
        (
            {'type': 'array', 'items': {'type': 'string'}, 'default': None},
            'std::optional<std::vector<std::string>>',
            '',
            '',
        ),
        (
            {'type': 'array', 'items': {'type': 'string'}, 'default': []},
            'std::vector<std::string>',
            '',
            'std::vector<std::string>{}',
        ),
        (
            {'type': 'array', 'items': {'type': 'string'}, 'default': ['a', 'q"\\\n\x01']},
            'std::vector<std::string>',
            '"a", "q\\"\\\\\\n\\001"',
            'std::vector<std::string>{"a", "q\\"\\\\\\n\\001"}',
        ),
        (
            {'type': 'array', 'items': {'type': 'integer'}, 'default': [1, -2]},
            'std::vector<int>',
            '1, -2',
            'std::vector<int>{1, -2}',
        ),
        (
            {'type': 'array', 'items': {'type': 'number'}, 'default': [1, 2.5]},
            'std::vector<double>',
            '1.0, 2.5',
            'std::vector<double>{1.0, 2.5}',
        ),
        (
            {'type': 'array', 'items': {'type': 'boolean'}, 'default': [True, False]},
            'std::vector<bool>',
            'true, false',
            'std::vector<bool>{true, false}',
        ),
        (
            {'type': 'array', 'items': {'type': 'string', 'enum': ['a-b']}, 'default': ['a-b']},
            'std::vector<::type::FieldA>',
            '::type::FieldA::kAB',
            'std::vector<::type::FieldA>{::type::FieldA::kAB}',
        ),
    ],
)
def test_array_field_default(simple_gen, array_schema, field_type, initializer, parse_default):
    field = _array_field(simple_gen, array_schema)
    assert field.cpp_field_type() == field_type
    assert field.get_default() == initializer
    assert field.get_parse_default() == parse_default
    assert not field.has_default_constant()


def test_array_field_default_descriptor(simple_gen):
    field = _array_field(simple_gen, {'type': 'array', 'items': {'type': 'integer'}, 'default': []})
    assert field.is_optional() is False
    assert field.descriptor_type('::type').startswith(
        'USERVER_NAMESPACE::chaotic::Field<::type, USERVER_NAMESPACE::chaotic::MemberDefaulted<'
    )


def test_array_field_user_type_default(simple_gen):
    field = _array_field(
        simple_gen,
        {'type': 'array', 'items': {'type': 'string'}, 'x-usrv-cpp-type': 'my::Tags', 'default': ['a']},
    )
    assert field.cpp_field_type() == 'my::Tags'
    assert field.get_default() == 'USERVER_NAMESPACE::chaotic::ConvertTo<my::Tags>(std::vector<std::string>{"a"})'


@pytest.mark.parametrize(
    'items,default,msg',
    [
        ({'type': 'integer'}, ['1'], '"default" item \'1\' does not match the items type "integer"'),
        ({'type': 'integer'}, [True], '"default" item True does not match the items type "integer"'),
        ({'type': 'boolean'}, [1], '"default" item 1 does not match the items type "boolean"'),
        ({'type': 'number'}, [float('inf')], '"default" item inf does not match the items type "number"'),
        ({'type': 'string', 'enum': ['a']}, ['b'], '"default" item \'b\' is not one of [\'a\']'),
        ({'type': 'string', 'format': 'uuid'}, ['x'], cpp_types.UNSUPPORTED_ARRAY_DEFAULT_ITEMS_MSG),
        ({'type': 'string', 'x-usrv-cpp-type': 'my::String'}, ['x'], cpp_types.UNSUPPORTED_ARRAY_DEFAULT_ITEMS_MSG),
        ({'type': 'array', 'items': {'type': 'integer'}}, [[1]], cpp_types.UNSUPPORTED_ARRAY_DEFAULT_ITEMS_MSG),
    ],
)
def test_array_invalid_default_items(simple_gen, items, default, msg):
    with pytest.raises(cpp_translator.TranslatorError) as exc:
        simple_gen({'type': 'array', 'items': items, 'default': default})
    assert exc.value.infile_path == '/definitions/type'
    assert exc.value.msg == msg


@pytest.mark.parametrize(
    'items',
    [
        {'type': 'string', 'format': 'uuid'},
        {'type': 'object', 'properties': {}, 'additionalProperties': False},
    ],
)
def test_array_empty_default_any_items(simple_gen, items):
    field = _array_field(simple_gen, {'type': 'array', 'items': items, 'default': []})
    assert field.get_default() == ''
