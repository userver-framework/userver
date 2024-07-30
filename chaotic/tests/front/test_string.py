from chaotic.front.types import String
from chaotic.front.types import StringFormat


def test_string_enum(simple_parse):
    parsed = simple_parse({'type': 'string', 'enum': ['some', 'thing']})
    assert parsed.schemas == {
        'vfull#/definitions/type': String(enum=['some', 'thing']),
    }


def test_string(simple_parse):
    parsed = simple_parse({'type': 'string'})
    assert parsed.schemas == {'vfull#/definitions/type': String()}


def test_string_format_datetime(simple_parse):
    parsed = simple_parse({'type': 'string', 'format': 'date-time'})
    assert parsed.schemas == {
        'vfull#/definitions/type': String(format=StringFormat.DATE_TIME),
    }


def test_string_format_uuid(simple_parse):
    parsed = simple_parse({'type': 'string', 'format': 'uuid'})
    assert parsed.schemas == {
        'vfull#/definitions/type': String(format=StringFormat.UUID),
    }
