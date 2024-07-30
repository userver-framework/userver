from chaotic.back.cpp.types import CppArray
from chaotic.back.cpp.types import CppArrayValidator
from chaotic.back.cpp.types import CppPrimitiveType
from chaotic.back.cpp.types import CppPrimitiveValidator


def test_array_int(simple_gen):
    types = simple_gen({'type': 'array', 'items': {'type': 'integer'}})
    assert types == {
        '/definitions/type': CppArray(
            raw_cpp_type='NOT_USED',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            items=CppPrimitiveType(
                raw_cpp_type='int',
                user_cpp_type=None,
                json_schema=None,
                nullable=False,
                validators=CppPrimitiveValidator(prefix='/definitions/typeA'),
            ),
            container='std::vector',
            validators=CppArrayValidator(),
        ),
    }


def test_array_array_with_validators(simple_gen):
    types = simple_gen(
        {
            'type': 'array',
            'items': {
                'type': 'array',
                'items': {'type': 'integer', 'minimum': 1},
            },
        },
    )
    assert types == {
        '/definitions/type': CppArray(
            raw_cpp_type='NOT_USED',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            items=CppArray(
                raw_cpp_type='NOT_USED',
                user_cpp_type=None,
                json_schema=None,
                nullable=False,
                items=CppPrimitiveType(
                    raw_cpp_type='int',
                    user_cpp_type=None,
                    json_schema=None,
                    nullable=False,
                    validators=CppPrimitiveValidator(
                        min=1, prefix='/definitions/typeAA',
                    ),
                ),
                container='std::vector',
                validators=CppArrayValidator(),
            ),
            container='std::vector',
            validators=CppArrayValidator(),
        ),
    }
