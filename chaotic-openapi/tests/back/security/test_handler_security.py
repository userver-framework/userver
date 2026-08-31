import yaml

from chaotic_openapi.back.cpp.handler import renderer as handler_renderer
from chaotic_openapi.back.cpp.handler import translator as handler_translator
from chaotic_openapi.back.cpp.handler.types import ServerSpec
from chaotic_openapi.front import parser as front_parser

_BASE = {
    'openapi': '3.0.0',
    'info': {'title': '', 'version': '1.0'},
}

_DIGEST_SCHEME = {'type': 'http', 'scheme': 'digest'}


def _translate_handler(openapi_doc: dict) -> ServerSpec:
    parser = front_parser.Parser('test')
    parser.parse_schema(openapi_doc, '<inline>', '<inline>')
    tr = handler_translator.HandlerTranslator(
        parser.service(),
        cpp_namespace='handlers::test',
        include_dirs=[],
    )
    return tr.spec()


def test_handler_config_yaml_with_digest_auth():
    """Digest security -> config.chaotic.yaml includes auth.types entry."""
    doc = dict(_BASE)
    doc['components'] = {'securitySchemes': {'myDigest': _DIGEST_SCHEME}}
    doc['paths'] = {
        '/test': {
            'get': {
                'operationId': 'getTest',
                'responses': {},
                'security': [{'myDigest': []}],
            },
        },
    }
    spec = _translate_handler(doc)
    config_yaml = handler_renderer.render_config_yaml(spec)
    config = yaml.safe_load(config_yaml)

    components = config['components_manager']['components']
    handler_entry = components['handler-get-test']
    assert handler_entry['auth'] == {'types': ['myDigest']}


def test_handler_config_yaml_no_security():
    """Operation without security -> no auth key in config.chaotic.yaml entry."""
    doc = dict(_BASE)
    doc['paths'] = {
        '/test': {
            'get': {
                'operationId': 'getTest',
                'responses': {},
            },
        },
    }
    spec = _translate_handler(doc)
    config_yaml = handler_renderer.render_config_yaml(spec)
    config = yaml.safe_load(config_yaml)

    components = config['components_manager']['components']
    handler_entry = components['handler-get-test']
    assert 'auth' not in handler_entry


def test_handler_config_yaml_mixed_ops():
    """Mixed secured/unsecured ops: only secured op gets auth key."""
    doc = dict(_BASE)
    doc['components'] = {'securitySchemes': {'myDigest': _DIGEST_SCHEME}}
    doc['paths'] = {
        '/secured': {
            'get': {
                'operationId': 'getSecured',
                'responses': {},
                'security': [{'myDigest': []}],
            },
        },
        '/public': {
            'get': {
                'operationId': 'getPublic',
                'responses': {},
            },
        },
    }
    spec = _translate_handler(doc)
    config_yaml = handler_renderer.render_config_yaml(spec)
    config = yaml.safe_load(config_yaml)

    components = config['components_manager']['components']
    assert components['handler-get-secured']['auth'] == {'types': ['myDigest']}
    assert 'auth' not in components['handler-get-public']
