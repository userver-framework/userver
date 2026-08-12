import pytest

from chaotic.front import parser as front_parser
from chaotic.front import types as front_types


def test_string_const(simple_parse):
    parsed = simple_parse({'type': 'string', 'const': 'active'})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const='active',
            const_type=front_types.ConstType.STRING,
        ),
    }


def test_integer_const(simple_parse):
    parsed = simple_parse({'type': 'integer', 'const': 42})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const=42,
            const_type=front_types.ConstType.INTEGER,
        ),
    }


def test_integer_const_int32(simple_parse):
    parsed = simple_parse({'type': 'integer', 'format': 'int32', 'const': 42})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const=42,
            const_type=front_types.ConstType.INTEGER32,
        ),
    }


def test_integer_const_int64(simple_parse):
    parsed = simple_parse({'type': 'integer', 'format': 'int64', 'const': 9000000000})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const=9000000000,
            const_type=front_types.ConstType.INTEGER64,
        ),
    }


def test_boolean_const_true(simple_parse):
    parsed = simple_parse({'type': 'boolean', 'const': True})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const=True,
            const_type=front_types.ConstType.BOOLEAN,
        ),
    }


def test_boolean_const_false(simple_parse):
    parsed = simple_parse({'type': 'boolean', 'const': False})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const=False,
            const_type=front_types.ConstType.BOOLEAN,
        ),
    }


def test_standalone_string_const(simple_parse):
    parsed = simple_parse({'const': 'active'})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const='active',
            const_type=front_types.ConstType.STRING,
        ),
    }


def test_standalone_integer_const(simple_parse):
    parsed = simple_parse({'const': 42})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const=42,
            const_type=front_types.ConstType.INTEGER,
        ),
    }


def test_standalone_boolean_const(simple_parse):
    parsed = simple_parse({'const': True})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const=True,
            const_type=front_types.ConstType.BOOLEAN,
        ),
    }


def test_const_allows_schema_metadata(simple_parse):
    parsed = simple_parse({
        'type': 'string',
        'const': 'active',
        'title': 'Status',
        'description': 'status',
        'example': 'active',
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.ConstSchema(
            const='active',
            const_type=front_types.ConstType.STRING,
            title='Status',
            description='status',
            example='active',
        ),
    }


def test_const_with_minlength_error(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'string', 'const': 'active', 'minLength': 1})
    assert '"const" cannot be combined with' in exc.value.msg


def test_const_with_minimum_error(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'integer', 'const': 42, 'minimum': 1})
    assert '"const" cannot be combined with' in exc.value.msg


def test_const_with_enum_error(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'string', 'const': 'active', 'enum': ['active', 'inactive']})
    assert '"const" cannot be combined with' in exc.value.msg


def test_integer_const_with_bool_value_error(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'integer', 'const': True})
    assert 'boolean' in exc.value.msg


def test_standalone_const_unsupported_type_error(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'const': 3.14})
    assert 'Unsupported const value type' in exc.value.msg
