from chaotic.back.cpp import type_name
from chaotic.back.cpp.types import CppArray
from chaotic.back.cpp.types import CppArrayValidator
from chaotic.back.cpp.types import CppPrimitiveType
from chaotic.back.cpp.types import CppPrimitiveValidator


def test_array_int(simple_gen):
    types = simple_gen({'type': 'array', 'items': {'type': 'integer'}})
    assert types == {
        '::type': CppArray(
            raw_cpp_type=type_name.TypeName('::type'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            items=CppPrimitiveType(
                raw_cpp_type=type_name.TypeName('int'),
                user_cpp_type=None,
                json_schema=None,
                nullable=False,
                validators=CppPrimitiveValidator(prefix='typeA'),
            ),
            container='std::vector',
            validators=CppArrayValidator(),
        ),
    }


def test_array_array_with_validators(simple_gen):
    types = simple_gen({
        'type': 'array',
        'items': {'type': 'array', 'items': {'type': 'integer', 'minimum': 1}},
    })
    assert types == {
        '::type': CppArray(
            raw_cpp_type=type_name.TypeName('::type'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            items=CppArray(
                raw_cpp_type=type_name.TypeName('::typeA'),
                user_cpp_type=None,
                json_schema=None,
                nullable=False,
                items=CppPrimitiveType(
                    raw_cpp_type=type_name.TypeName('int'),
                    user_cpp_type=None,
                    json_schema=None,
                    nullable=False,
                    validators=CppPrimitiveValidator(
                        min=1,
                        prefix='typeAA',
                    ),
                ),
                container='std::vector',
                validators=CppArrayValidator(),
            ),
            container='std::vector',
            validators=CppArrayValidator(),
        ),
    }
