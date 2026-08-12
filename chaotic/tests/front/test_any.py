from chaotic.front import types as front_types


def test_any_value(simple_parse):
    parsed = simple_parse({})
    assert parsed.schemas == {'vfull#/definitions/type': front_types.AnyValue()}


def test_any_value_with_description(simple_parse):
    parsed = simple_parse({'description': 'any json value'})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.AnyValue(description='any json value'),
    }
