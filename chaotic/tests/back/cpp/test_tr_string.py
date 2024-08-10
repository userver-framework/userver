from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types


def test_simple(simple_gen):
    types = simple_gen({'type': 'string'})
    assert types == {
        '/definitions/type': cpp_types.CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('std::string'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=cpp_types.CppPrimitiveValidator(
                prefix='/definitions/type',
            ),
        ),
    }


def test_enum(simple_gen):
    types = simple_gen({'type': 'string', 'enum': ['foo', 'bar']})
    assert types == {
        '/definitions/type': cpp_types.CppStringEnum(
            raw_cpp_type=type_name.TypeName('/definitions/type'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            name='/definitions/type',
            default=None,
            enums=[
                cpp_types.CppStringEnumItem(raw_name='foo', cpp_name='kFoo'),
                cpp_types.CppStringEnumItem(raw_name='bar', cpp_name='kBar'),
            ],
        ),
    }


def test_datetime(simple_gen):
    types = simple_gen({'type': 'string', 'format': 'date-time'})
    assert types == {
        '/definitions/type': cpp_types.CppStringWithFormat(
            raw_cpp_type=type_name.TypeName('std::string'),
            format_cpp_type='userver::utils::datetime::TimePointTz',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            default=None,
        ),
    }


def test_datetime_isobasic(simple_gen):
    types = simple_gen({'type': 'string', 'format': 'date-time-iso-basic'})
    assert types == {
        '/definitions/type': cpp_types.CppStringWithFormat(
            raw_cpp_type=type_name.TypeName('std::string'),
            format_cpp_type='userver::utils::datetime::TimePointTzIsoBasic',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            default=None,
        ),
    }
