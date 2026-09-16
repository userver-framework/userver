import pytest

from chaotic import error as chaotic_error
from chaotic_openapi.back.cpp.client import translator
from chaotic_openapi.back.cpp.common import types as common_types
from chaotic_openapi.front import parser as front_parser


def _make_translator(openapi_doc: dict) -> translator.Translator:
    parser = front_parser.Parser('test')
    parser.parse_schema(openapi_doc, '<inline>', '<inline>')
    return translator.Translator(
        parser.service(),
        dynamic_config='',
        cpp_namespace='test_namespace',
        include_dirs=[],
        middleware_plugins=[],
    )


def _single_op_security(openapi_doc: dict) -> common_types.Security | None:
    spec = _make_translator(openapi_doc).spec()
    assert len(spec.operations) == 1
    return spec.operations[0].security


_BASE = {
    'openapi': '3.0.0',
    'info': {'title': '', 'version': '1.0'},
}

_DIGEST_SCHEME = {'type': 'http', 'scheme': 'digest'}
_BASIC_SCHEME = {'type': 'http', 'scheme': 'basic'}
_BEARER_SCHEME = {'type': 'http', 'scheme': 'bearer'}
_APIKEY_SCHEME = {'type': 'apiKey', 'name': 'X-Api-Key', 'in': 'header'}


def _openapi_with_scheme(scheme_name: str, scheme_def: dict, op_security: list[dict] | None = None) -> dict:
    doc = dict(_BASE)
    doc['components'] = {'securitySchemes': {scheme_name: scheme_def}}
    doc['paths'] = {
        '/test': {
            'get': {
                'responses': {},
                'security': op_security if op_security is not None else [{scheme_name: []}],
            },
        },
    }
    return doc


def test_digest_security_translated():
    """http/digest -> HttpDigestSecurity with auth_type = scheme name."""
    sec = _single_op_security(_openapi_with_scheme('myDigest', _DIGEST_SCHEME))
    assert isinstance(sec, common_types.HttpDigestSecurity)
    assert sec.auth_type == 'myDigest'


def test_digest_security_case_insensitive_scheme():
    """scheme field is normalised to lowercase by the front parser."""
    sec = _single_op_security(_openapi_with_scheme('MyDigest', {'type': 'http', 'scheme': 'Digest'}))
    assert isinstance(sec, common_types.HttpDigestSecurity)
    assert sec.auth_type == 'MyDigest'


def test_no_security():
    """Operation with no security -> security is None."""
    doc = dict(_BASE)
    doc['paths'] = {'/test': {'get': {'responses': {}}}}
    sec = _single_op_security(doc)
    assert sec is None


def test_two_ops_different_digest_schemes():
    """Two operations with different digest schemes produce distinct auth_types."""
    doc = dict(_BASE)
    doc['components'] = {
        'securitySchemes': {
            'adminDigest': _DIGEST_SCHEME,
            'userDigest': _DIGEST_SCHEME,
        },
    }
    doc['paths'] = {
        '/admin': {'get': {'responses': {}, 'security': [{'adminDigest': []}]}},
        '/user': {'get': {'responses': {}, 'security': [{'userDigest': []}]}},
    }
    parser = front_parser.Parser('test')
    parser.parse_schema(doc, '<inline>', '<inline>')
    spec = translator.Translator(
        parser.service(),
        dynamic_config='',
        cpp_namespace='test_namespace',
        include_dirs=[],
        middleware_plugins=[],
    ).spec()

    assert len(spec.operations) == 2
    auth_types = {op.security.auth_type for op in spec.operations}  # type: ignore[union-attr]
    assert auth_types == {'adminDigest', 'userDigest'}

    assert spec.uses_digest_security()
    assert spec.digest_scheme_names() == ['adminDigest', 'userDigest']


def test_and_security_rejected():
    """AND semantics (multiple keys in one security object) must raise."""
    doc = dict(_BASE)
    doc['components'] = {
        'securitySchemes': {
            'digestAuth': _DIGEST_SCHEME,
            'digestAuth2': _DIGEST_SCHEME,
        },
    }
    doc['paths'] = {
        '/test': {
            'get': {
                'responses': {},
                # Two keys in one dict = AND requirement
                'security': [{'digestAuth': [], 'digestAuth2': []}],
            },
        },
    }
    with pytest.raises(chaotic_error.BaseError, match=r'.*AND semantics.*'):
        _make_translator(doc)


def test_or_security_rejected():
    doc = dict(_BASE)
    doc['components'] = {
        'securitySchemes': {
            'digestAuth': _DIGEST_SCHEME,
            'digestAuth2': _DIGEST_SCHEME,
        },
    }
    doc['paths'] = {
        '/test': {
            'get': {
                'responses': {},
                'security': [{'digestAuth': []}, {'digestAuth2': []}],
            },
        },
    }
    with pytest.raises(chaotic_error.BaseError, match=r'.*OR semantics.*'):
        _make_translator(doc)


def test_explicit_empty_security_overrides_global_security():
    doc = _openapi_with_scheme('digestAuth', _DIGEST_SCHEME, op_security=[])
    doc['security'] = [{'digestAuth': []}]
    assert _single_op_security(doc) is None


def test_anonymous_security_requirement_is_unsecured():
    assert _single_op_security(_openapi_with_scheme('digestAuth', _DIGEST_SCHEME, op_security=[{}])) is None


def test_http_basic_rejected():
    """http/basic raises a "not yet supported" error."""
    with pytest.raises(chaotic_error.BaseError, match=r'.*http/basic.*not yet supported.*'):
        _make_translator(_openapi_with_scheme('basicAuth', _BASIC_SCHEME))


def test_http_bearer_rejected():
    """http/bearer raises a "not yet supported" error."""
    with pytest.raises(chaotic_error.BaseError, match=r'.*http/bearer.*not yet supported.*'):
        _make_translator(_openapi_with_scheme('bearerAuth', _BEARER_SCHEME))


def test_apikey_rejected():
    """apiKey raises a "not yet supported" error."""
    with pytest.raises(chaotic_error.BaseError, match=r'.*apiKey.*not yet supported.*'):
        _make_translator(_openapi_with_scheme('apiKeyAuth', _APIKEY_SCHEME))
