from chaotic.back.cpp import types


def test_camel_to_snake_case_smoke():
    assert types.camel_to_snake_case('std/chrono') == 'std/chrono'
    assert (
        types.camel_to_snake_case('geometry/Distance') == 'geometry/distance'
    )
    assert types.camel_to_snake_case('unsigned') == 'unsigned'


def test_get_include_by_cpp_type_smoke():
    assert types.CppType.get_include_by_cpp_type('geometry::Distance') == [
        'userver/chaotic/io/geometry/distance.hpp',
    ]
    assert types.CppType.get_include_by_cpp_type('std::vector<xxx>') == [
        'userver/chaotic/io/std/vector.hpp',
    ]
    assert types.CppType.get_include_by_cpp_type(
        'userver::utils::StrongTypedef<xxx, std::string>',
    ) == ['userver/chaotic/io/xxx.hpp']
