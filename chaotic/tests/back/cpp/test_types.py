from chaotic.back.cpp import type_name
from chaotic.back.cpp import types
from chaotic.front import types as front_types


def test_camel_to_snake_case_smoke():
    assert types.camel_to_snake_case('std/chrono') == 'std/chrono'
    assert types.camel_to_snake_case('geometry/Distance') == 'geometry/distance'
    assert types.camel_to_snake_case('unsigned') == 'unsigned'


def test_get_includes_by_cpp_type_smoke():
    assert types.CppType.get_includes_by_cpp_type('geometry::Distance') == [
        'userver/chaotic/io/geometry/distance.hpp',
    ]
    assert types.CppType.get_includes_by_cpp_type('std::vector<xxx>') == [
        'userver/chaotic/io/std/vector.hpp',
    ]
    assert types.CppType.get_includes_by_cpp_type(
        'userver::utils::StrongTypedef<xxx, std::string>',
    ) == ['userver/utils/strong_typedef.hpp', 'userver/chaotic/io/xxx.hpp']


def test_need_to_json_string():
    common_kwargs = {
        'raw_cpp_type': type_name.TypeName('::type'),
        'json_schema': front_types.Schema(),
        'nullable': False,
        'user_cpp_type': None,
    }

    assert not types.CppType(**common_kwargs).need_to_json_string()
    assert types.CppStruct(fields={}, **common_kwargs).need_to_json_string()
    assert types.CppStructAllOf(parents=[], **common_kwargs).need_to_json_string()
