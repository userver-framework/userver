from chaotic import error
from chaotic.back.cpp import type_name
from chaotic.back.cpp.types import CppIntEnum
from chaotic.back.cpp.types import CppIntEnumItem
from chaotic.back.cpp.types import CppPrimitiveType
from chaotic.back.cpp.types import CppPrimitiveValidator


def test_int(simple_gen):
    types = simple_gen({'type': 'integer'})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='type'),
        ),
    }


def test_wrong_type_x(simple_gen):
    try:
        simple_gen({'type': 'integer', 'x-usrv-cpp-type': 1})
        assert False
    except error.BaseError as exc:
        assert exc.full_filepath == 'vfull'
        assert exc.infile_path == '/definitions/type'
        assert exc.schema_type == 'jsonschema'
        assert exc.msg == 'Non-string x- property "x-usrv-cpp-type"'


def test_int_nullable(simple_gen):
    types = simple_gen({'type': 'integer', 'nullable': True})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            json_schema=None,
            nullable=True,
            validators=CppPrimitiveValidator(prefix='type'),
        ),
    }


def test_int_cpp_type(simple_gen):
    types = simple_gen({'type': 'integer', 'x-usrv-cpp-type': 'X'})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type='X',
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='type'),
        ),
    }


def test_int_default(simple_gen):
    types = simple_gen({'type': 'integer', 'default': 42})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            default=42,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='type'),
        ),
    }


def test_int_min(simple_gen):
    types = simple_gen({'type': 'integer', 'minimum': 1})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(
                min=1,
                prefix='type',
            ),
        ),
    }


def test_int_min_max(simple_gen):
    types = simple_gen({'type': 'integer', 'minimum': 1, 'maximum': 10})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(
                min=1,
                max=10,
                prefix='type',
            ),
            nullable=False,
        ),
    }


def test_int_min_max_exclusive(simple_gen):
    types = simple_gen({
        'type': 'integer',
        'exclusiveMinimum': 1,
        'exclusiveMaximum': 10,
    })
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(
                exclusiveMin=1,
                exclusiveMax=10,
                prefix='type',
            ),
            nullable=False,
        ),
    }


def test_int_min_max_exclusive_false(simple_gen):
    types = simple_gen({
        'type': 'integer',
        'exclusiveMinimum': False,
        'exclusiveMaximum': False,
    })
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(prefix='type'),
            nullable=False,
        ),
    }


def test_int_min_max_exclusive_legacy(simple_gen):
    types = simple_gen({
        'type': 'integer',
        'exclusiveMinimum': True,
        'minimum': 2,
        'exclusiveMaximum': True,
        'maximum': 10,
    })
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('int'),
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(
                exclusiveMin=2,
                exclusiveMax=10,
                prefix='type',
            ),
            nullable=False,
        ),
    }


def test_int_format_int32(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int32'})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('std::int32_t'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='type'),
        ),
    }


def test_int_format_int64(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int64'})
    assert types == {
        '::type': CppPrimitiveType(
            raw_cpp_type=type_name.TypeName('std::int64_t'),
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='type'),
        ),
    }


def test_int_enum(simple_gen):
    types = simple_gen({
        'type': 'integer',
        'enum': [0, 1, 2, 3, 5],
        'x-enum-varnames': ['CamelCase', 'snake_case', 'UPPER', 'lower'],
    })
    assert types == {
        '::type': CppIntEnum(
            raw_cpp_type=type_name.TypeName('::type'),
            user_cpp_type=None,
            name='::type',
            json_schema=None,
            nullable=False,
            enums=[
                CppIntEnumItem(value=0, raw_name='CamelCase', cpp_name='CamelCase'),
                CppIntEnumItem(value=1, raw_name='snake_case', cpp_name='SnakeCase'),
                CppIntEnumItem(value=2, raw_name='UPPER', cpp_name='Upper'),
                CppIntEnumItem(value=3, raw_name='lower', cpp_name='Lower'),
                CppIntEnumItem(value=5, raw_name='5', cpp_name='5'),
            ],
        ),
    }
