import pytest

from chaotic import error
from chaotic.back.cpp.types import CppPrimitiveType
from chaotic.back.cpp.types import CppPrimitiveValidator
from chaotic.back.cpp.types import CppStruct
from chaotic.back.cpp.types import CppStructField
from chaotic.back.cpp.types import CppStructPrimitiveField


def test_empty(simple_gen):
    schemas = simple_gen(
        {'type': 'object', 'properties': {}, 'additionalProperties': False},
    )
    assert schemas == {
        '/definitions/type': CppStruct(
            raw_cpp_type='/definitions/type',
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            fields={},
        ),
    }


def test_additional_properties_simple(simple_gen):
    schemas = simple_gen(
        {
            'type': 'object',
            'properties': {},
            'additionalProperties': {'type': 'integer'},
        },
    )
    assert schemas == {
        '/definitions/type': CppStruct(
            raw_cpp_type='/definitions/type',
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            fields={},
            extra_type=CppPrimitiveType(
                raw_cpp_type='int',
                json_schema=None,
                nullable=False,
                user_cpp_type=None,
                validators=CppPrimitiveValidator(
                    namespace='/definitions/type', prefix='Extra',
                ),
            ),
        ),
    }


@pytest.mark.skip(reason='see comment in translator.py: _gen_field()')
def test_field_external(simple_gen):
    schemas = simple_gen(
        {
            'type': 'object',
            'properties': {'field': {'type': 'integer'}},
            'additionalProperties': False,
        },
    )
    assert schemas == {
        '/definitions/type': CppStruct(
            raw_cpp_type='/definitions/type',
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': CppStructField(
                    name='field',
                    required=False,
                    external_schema=CppStructPrimitiveField(
                        raw_cpp_type='int',
                    ),
                ),
            },
        ),
    }


def test_field_with_default(simple_gen):
    schemas = simple_gen(
        {
            'type': 'object',
            'properties': {'field': {'type': 'integer', 'default': 1}},
            'additionalProperties': False,
        },
    )
    assert schemas == {
        '/definitions/type': CppStruct(
            raw_cpp_type='/definitions/type',
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': CppStructField(
                    name='field',
                    required=False,
                    schema=CppPrimitiveType(
                        raw_cpp_type='int',
                        json_schema=None,
                        default=1,
                        nullable=False,
                        user_cpp_type=None,
                        validators=CppPrimitiveValidator(
                            namespace='/definitions/type', prefix='Field',
                        ),
                    ),
                ),
            },
        ),
    }


def test_field_inplace(simple_gen):
    schemas = simple_gen(
        {
            'type': 'object',
            'properties': {'field': {'type': 'integer', 'minimum': 1}},
            'additionalProperties': False,
        },
    )
    assert schemas == {
        '/definitions/type': CppStruct(
            raw_cpp_type='/definitions/type',
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': CppStructField(
                    name='field',
                    required=False,
                    schema=CppPrimitiveType(
                        raw_cpp_type='int',
                        json_schema=None,
                        nullable=False,
                        user_cpp_type=None,
                        validators=CppPrimitiveValidator(
                            min=1,
                            namespace='/definitions/type',
                            prefix='Field',
                        ),
                    ),
                ),
            },
        ),
    }


def test_field_is_struct(simple_gen):
    schemas = simple_gen(
        {
            'type': 'object',
            'properties': {
                'field': {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
            },
            'additionalProperties': False,
        },
    )
    assert schemas == {
        '/definitions/type': CppStruct(
            raw_cpp_type='/definitions/type',
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': CppStructField(
                    name='field',
                    required=False,
                    schema=CppStruct(
                        raw_cpp_type='/definitions/type@Field',
                        json_schema=None,
                        nullable=False,
                        user_cpp_type=None,
                        fields={},
                    ),
                ),
            },
        ),
    }


def test_field_required(simple_gen):
    schemas = simple_gen(
        {
            'type': 'object',
            'properties': {'field': {'type': 'integer', 'minimum': 1}},
            'required': ['field'],
            'additionalProperties': False,
        },
    )
    assert schemas == {
        '/definitions/type': CppStruct(
            raw_cpp_type='/definitions/type',
            json_schema=None,
            nullable=False,
            user_cpp_type=None,
            # name='vfull#/definitions/type',
            fields={
                'field': CppStructField(
                    name='field',
                    required=True,
                    schema=CppPrimitiveType(
                        raw_cpp_type='int',
                        json_schema=None,
                        nullable=False,
                        user_cpp_type=None,
                        validators=CppPrimitiveValidator(
                            min=1,
                            namespace='/definitions/type',
                            prefix='Field',
                        ),
                    ),
                ),
            },
        ),
    }


def test_extra_member_nonboolean(simple_gen):
    try:
        simple_gen(
            {
                'type': 'object',
                'properties': {},
                'x-taxi-cpp-extra-member': False,
                'additionalProperties': {'type': 'integer'},
            },
        )
        assert False
    except error.BaseError as exc:
        assert exc.msg == (
            '"x-usrv-cpp-extra-member: false" is not allowed for non-boolean '
            '"additionalProperties"'
        )
