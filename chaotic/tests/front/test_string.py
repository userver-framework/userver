from chaotic.front import types as front_types


def test_string_enum(simple_parse):
    parsed = simple_parse({'type': 'string', 'enum': ['some', 'thing']})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.String(enum=['some', 'thing']),
    }


def test_string(simple_parse):
    parsed = simple_parse({'type': 'string'})
    assert parsed.schemas == {'vfull#/definitions/type': front_types.String()}


def test_string_format_datetime(simple_parse):
    parsed = simple_parse({'type': 'string', 'format': 'date-time'})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.String(format=front_types.StringFormat.DATE_TIME),
    }


def test_string_format_uuid(simple_parse):
    parsed = simple_parse({'type': 'string', 'format': 'uuid'})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.String(format=front_types.StringFormat.UUID),
    }


def test_string_format_byte(simple_parse):
    parsed = simple_parse({'type': 'string', 'format': 'byte'})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.String(format=front_types.StringFormat.BYTE),
    }
