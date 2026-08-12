from chaotic.back.cpp import types as cpp_types


def test_boolean(simple_gen, cpp_primitive_type):
    types = simple_gen({'type': 'boolean'})
    assert types == {
        '::type': cpp_primitive_type(
            validators=cpp_types.CppPrimitiveValidator(),
            raw_cpp_type_str='bool',
        ),
    }
