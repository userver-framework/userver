from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types


def test_raw_json_string_without_type(simple_gen):
    types = simple_gen({'x-usrv-cpp-type': 'userver::formats::json::RawString'})
    assert types == {
        '::type': cpp_types.CppAnyValue(
            raw_cpp_type=type_name.TypeName('USERVER_NAMESPACE::formats::json::Value'),
            user_cpp_type='userver::formats::json::RawString',
            json_schema=front_types.Schema(),
            nullable=False,
        ),
    }


def test_raw_json_string_without_type_short_form(simple_gen):
    types = simple_gen({'x-usrv-cpp-type': 'formats::json::RawString'})
    assert types == {
        '::type': cpp_types.CppAnyValue(
            raw_cpp_type=type_name.TypeName('USERVER_NAMESPACE::formats::json::Value'),
            user_cpp_type='formats::json::RawString',
            json_schema=front_types.Schema(),
            nullable=False,
        ),
    }
