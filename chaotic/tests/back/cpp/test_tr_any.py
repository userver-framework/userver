from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types


def test_any_value(simple_gen):
    types = simple_gen({})
    assert types == {
        '::type': cpp_types.CppAnyValue(
            raw_cpp_type=type_name.TypeName(
                'USERVER_NAMESPACE::formats::json::Value',
            ),
            user_cpp_type=None,
            json_schema=front_types.Schema(),
            nullable=False,
        ),
    }


def test_any_value_with_description(simple_gen):
    types = simple_gen({'description': 'any json value'})
    assert types == {
        '::type': cpp_types.CppAnyValue(
            raw_cpp_type=type_name.TypeName(
                'USERVER_NAMESPACE::formats::json::Value',
            ),
            user_cpp_type=None,
            json_schema=front_types.Schema(),
            nullable=False,
        ),
    }
