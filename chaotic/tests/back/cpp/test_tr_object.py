import pytest

from chaotic import error
from chaotic.back.cpp import translator
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types


def test_empty(simple_gen):
    schemas = simple_gen({
        'type': 'object',
        'properties': {},
        'additionalProperties': False,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            fields={},
        ),
    }, f'Generated schema is: {schemas}'


def test_very_empty(simple_gen):
    schemas = simple_gen({'type': 'object', 'additionalProperties': False})
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            fields={},
        ),
    }, f'Generated schema is: {schemas}'


def test_default_additional_properties(simple_gen):
    schemas = simple_gen({
        'type': 'object',
        'properties': {},
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            strict_parsing=False,
            fields={},
        ),
    }, f'Generated schema is: {schemas}'


def test_additional_properties_true(simple_gen):
    schemas = simple_gen({
        'type': 'object',
        'properties': {},
        'additionalProperties': True,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            extra_type=True,
            strict_parsing=True,
            fields={},
        ),
    }, f'Generated schema is: {schemas}'


def test_property_and_additional(simple_gen, cpp_primitive_type):
    schemas = simple_gen({
        'type': 'object',
        'properties': {'field': {'type': 'integer'}},
        'additionalProperties': {'type': 'boolean'},
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            fields={
                'field': cpp_types.CppStructField(
                    name='field',
                    required=False,
                    schema=cpp_primitive_type(
                        validators=cpp_types.CppPrimitiveValidator(
                            namespace='::type',
                            prefix='Field',
                        ),
                        raw_cpp_type_str='int',
                    ),
                ),
            },
            extra_type=cpp_primitive_type(
                validators=cpp_types.CppPrimitiveValidator(),
                raw_cpp_type_str='bool',
            ),
        ),
    }, f'Generated schema is: {schemas}'


def test_optional_nullable(simple_gen):
    with pytest.raises(translator.TranslatorError) as exc:
        simple_gen({
            'type': 'object',
            'properties': {
                'foo': {
                    'type': 'integer',
                    'nullable': True,
                },
            },
            'additionalProperties': False,
        })
    assert str(exc.value.msg) == 'optional nullable fields are not supported'


def test_additional_properties_simple(simple_gen, cpp_primitive_type):
    schemas = simple_gen({
        'type': 'object',
        'properties': {},
        'additionalProperties': {'type': 'integer'},
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            fields={},
            extra_type=cpp_primitive_type(
                validators=cpp_types.CppPrimitiveValidator(
                    namespace='::type',
                    prefix='Extra',
                ),
                raw_cpp_type_str='int',
            ),
        ),
    }, f'Generated schema is: {schemas}'


@pytest.mark.skip(reason='see comment in translator.py: _gen_field()')
def test_field_external(simple_gen, cpp_primitive_type):
    schemas = simple_gen({
        'type': 'object',
        'properties': {'field': {'type': 'integer'}},
        'additionalProperties': False,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': cpp_types.CppStructField(
                    name='field',
                    required=False,
                    schema=cpp_primitive_type(
                        validators=cpp_types.CppPrimitiveValidator(
                            namespace='::type',
                            prefix='Field',
                        ),
                        raw_cpp_type_str='int',
                    ),
                ),
            },
        ),
    }, f'Generated schema is: {schemas}'


def test_field_with_default(simple_gen, cpp_primitive_type):
    schemas = simple_gen({
        'type': 'object',
        'properties': {'field': {'type': 'integer', 'default': 1}},
        'additionalProperties': False,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': cpp_types.CppStructField(
                    name='field',
                    required=False,
                    schema=cpp_primitive_type(
                        validators=cpp_types.CppPrimitiveValidator(
                            namespace='::type',
                            prefix='Field',
                        ),
                        raw_cpp_type_str='int',
                        default=1,
                    ),
                ),
            },
        ),
    }, f'Generated schema is: {schemas}'


def test_field_escaping(simple_gen, cpp_primitive_type):
    schemas = simple_gen({
        'type': 'object',
        'properties': {
            '🙂🔥': {'type': 'integer', 'default': 1},
            'new': {'type': 'integer', 'default': 1},
            'with#white space!': {'type': 'integer', 'default': 1},
        },
        'additionalProperties': False,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                '🙂🔥': cpp_types.CppStructField(
                    name='u1F642_u1F525',
                    required=False,
                    schema=cpp_primitive_type(
                        validators=cpp_types.CppPrimitiveValidator(
                            namespace='::type',
                            prefix='u1F642_u1F525_',
                        ),
                        raw_cpp_type_str='int',
                        default=1,
                    ),
                ),
                'new': cpp_types.CppStructField(
                    name='new_',
                    required=False,
                    schema=cpp_primitive_type(
                        validators=cpp_types.CppPrimitiveValidator(
                            namespace='::type',
                            prefix='New',
                        ),
                        raw_cpp_type_str='int',
                        default=1,
                    ),
                ),
                'with#white space!': cpp_types.CppStructField(
                    name='with_white_space_',
                    required=False,
                    schema=cpp_primitive_type(
                        validators=cpp_types.CppPrimitiveValidator(
                            namespace='::type',
                            prefix='With_White_Space_',
                        ),
                        raw_cpp_type_str='int',
                        default=1,
                    ),
                ),
            },
        ),
    }, f'Generated schema is: {schemas}'


def test_field_inplace(simple_gen, cpp_primitive_type):
    schemas = simple_gen({
        'type': 'object',
        'properties': {'field': {'type': 'integer', 'minimum': 1}},
        'additionalProperties': False,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': cpp_types.CppStructField(
                    name='field',
                    required=False,
                    schema=cpp_primitive_type(
                        validators=cpp_types.CppPrimitiveValidator(
                            min=1,
                            namespace='::type',
                            prefix='Field',
                        ),
                        raw_cpp_type_str='int',
                    ),
                ),
            },
        ),
    }, f'Generated schema is: {schemas}'


def test_field_is_struct(simple_gen):
    schemas = simple_gen({
        'type': 'object',
        'properties': {
            'field': {
                'type': 'object',
                'properties': {},
                'additionalProperties': False,
            },
        },
        'additionalProperties': False,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': cpp_types.CppStructField(
                    name='field',
                    required=False,
                    schema=cpp_types.CppStruct(
                        raw_cpp_type=type_name.TypeName(
                            '::type::Field',
                        ),
                        json_schema=front_types.Schema(),
                        nullable=False,
                        user_cpp_type=None,
                        fields={},
                    ),
                ),
            },
        ),
    }, f'Generated schema is: {schemas}'


def test_field_required(simple_gen):
    schemas = simple_gen({
        'type': 'object',
        'properties': {'field': {'type': 'integer', 'minimum': 1}},
        'required': ['field'],
        'additionalProperties': False,
    })
    assert schemas == {
        '::type': cpp_types.CppStruct(
            raw_cpp_type=type_name.TypeName('::type'),
            json_schema=front_types.Schema(),
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': cpp_types.CppStructField(
                    name='field',
                    required=True,
                    schema=cpp_types.CppPrimitiveType(
                        raw_cpp_type=type_name.TypeName('int'),
                        json_schema=front_types.Schema(),
                        nullable=False,
                        user_cpp_type=None,
                        validators=cpp_types.CppPrimitiveValidator(
                            min=1,
                            namespace='::type',
                            prefix='Field',
                        ),
                    ),
                ),
            },
        ),
    }, f'Generated schema is: {schemas}'


def test_extra_member_nonboolean(simple_gen):
    try:
        simple_gen({
            'type': 'object',
            'properties': {},
            'x-taxi-extra-member': False,
            'additionalProperties': {'type': 'integer'},
        })
        assert False
    except error.BaseError as exc:
        assert exc.msg == ('"x-usrv-extra-member: false" is not allowed for non-boolean "additionalProperties"')
