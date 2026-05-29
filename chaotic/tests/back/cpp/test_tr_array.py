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
