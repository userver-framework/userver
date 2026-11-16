import pytest

from chaotic.back.cpp import translator
from chaotic.back.cpp import types as cpp_types
from chaotic.front import parser


@pytest.mark.parametrize('tag', ['x-usrv-with-fallback', 'x-taxi-with-fallback'])
@pytest.mark.parametrize('enabled', [True, False])
def test_enum_fallback(simple_gen, tag, enabled):
    schema = simple_gen({
        'type': 'string',
        'enum': ['unknown', 'order', 'shipment'],
        tag: enabled,
        'default': 'shipment',
    })['::type']
    assert isinstance(schema, cpp_types.CppStringEnum)
    assert schema.with_fallback is enabled
    assert [item.cpp_name for item in schema.enums] == ['kUnknown', 'kOrder', 'kShipment']
    assert schema.default == '::type::kShipment'


def test_closed_enum_is_default(simple_gen):
    schema = simple_gen({'type': 'string', 'enum': ['order', 'shipment']})['::type']
    assert not schema.with_fallback


@pytest.mark.parametrize('enabled', [True, False])
def test_matching_aliases(simple_gen, enabled):
    schema = simple_gen({
        'type': 'string',
        'enum': ['unknown', 'order'],
        'x-usrv-with-fallback': enabled,
        'x-taxi-with-fallback': enabled,
    })['::type']
    assert schema.with_fallback is enabled


@pytest.mark.parametrize('tag', ['x-usrv-with-fallback', 'x-taxi-with-fallback'])
@pytest.mark.parametrize('value', [None, 0, 1, 'true', [], {}])
def test_non_boolean_fallback(simple_gen, tag, value):
    with pytest.raises(translator.TranslatorError, match=f'{tag} must be a boolean'):
        simple_gen({'type': 'string', 'enum': ['unknown'], tag: value})


@pytest.mark.parametrize('enabled', [True, False])
def test_conflicting_aliases(simple_gen, enabled):
    with pytest.raises(translator.TranslatorError, match='must have the same value'):
        simple_gen({
            'type': 'string',
            'enum': ['unknown'],
            'x-usrv-with-fallback': enabled,
            'x-taxi-with-fallback': not enabled,
        })


@pytest.mark.parametrize(
    'schema',
    [
        {'type': 'string'},
        {'type': 'string', 'enum': []},
    ],
)
def test_requires_string_enum(simple_gen, schema):
    with pytest.raises(translator.TranslatorError, match='requires type: string and a non-empty enum'):
        simple_gen({**schema, 'x-usrv-with-fallback': True})


@pytest.mark.parametrize('type_', ['integer', 'boolean', 'object'])
def test_fallback_on_other_types_is_rejected(simple_gen, type_):
    with pytest.raises(parser.ParserError, match='with-fallback is not allowed in this context'):
        simple_gen({'type': type_, 'x-usrv-with-fallback': True})
