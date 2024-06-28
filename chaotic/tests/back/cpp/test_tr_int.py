from chaotic import error
from chaotic.back.cpp.types import CppIntEnum
from chaotic.back.cpp.types import CppPrimitiveType
from chaotic.back.cpp.types import CppPrimitiveValidator


def test_int(simple_gen):
    types = simple_gen({'type': 'integer'})
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='/definitions/type'),
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
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            json_schema=None,
            nullable=True,
            validators=CppPrimitiveValidator(prefix='/definitions/type'),
        ),
    }


def test_int_cpp_type(simple_gen):
    types = simple_gen({'type': 'integer', 'x-usrv-cpp-type': 'X'})
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type='X',
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='/definitions/type'),
        ),
    }


def test_int_default(simple_gen):
    types = simple_gen({'type': 'integer', 'default': 42})
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            default=42,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='/definitions/type'),
        ),
    }


def test_int_min(simple_gen):
    types = simple_gen({'type': 'integer', 'minimum': 1})
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(
                min=1, prefix='/definitions/type',
            ),
        ),
    }


def test_int_min_max(simple_gen):
    types = simple_gen({'type': 'integer', 'minimum': 1, 'maximum': 10})
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(
                min=1, max=10, prefix='/definitions/type',
            ),
            nullable=False,
        ),
    }


def test_int_min_max_exclusive(simple_gen):
    types = simple_gen(
        {'type': 'integer', 'exclusiveMinimum': 1, 'exclusiveMaximum': 10},
    )
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(
                exclusiveMin=1, exclusiveMax=10, prefix='/definitions/type',
            ),
            nullable=False,
        ),
    }


def test_int_min_max_exclusive_false(simple_gen):
    types = simple_gen(
        {
            'type': 'integer',
            'exclusiveMinimum': False,
            'exclusiveMaximum': False,
        },
    )
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(prefix='/definitions/type'),
            nullable=False,
        ),
    }


def test_int_min_max_exclusive_legacy(simple_gen):
    types = simple_gen(
        {
            'type': 'integer',
            'exclusiveMinimum': True,
            'minimum': 2,
            'exclusiveMaximum': True,
            'maximum': 10,
        },
    )
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='int',
            user_cpp_type=None,
            json_schema=None,
            validators=CppPrimitiveValidator(
                exclusiveMin=2, exclusiveMax=10, prefix='/definitions/type',
            ),
            nullable=False,
        ),
    }


def test_int_format_int32(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int32'})
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='std::int32_t',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='/definitions/type'),
        ),
    }


def test_int_format_int64(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int64'})
    assert types == {
        '/definitions/type': CppPrimitiveType(
            raw_cpp_type='std::int64_t',
            user_cpp_type=None,
            json_schema=None,
            nullable=False,
            validators=CppPrimitiveValidator(prefix='/definitions/type'),
        ),
    }


def test_int_enum(simple_gen):
    types = simple_gen({'type': 'integer', 'enum': [1, 2, 3]})
    assert types == {
        '/definitions/type': CppIntEnum(
            raw_cpp_type='/definitions/type',
            user_cpp_type=None,
            name='/definitions/type',
            json_schema=None,
            nullable=False,
            enums=[1, 2, 3],
        ),
    }
