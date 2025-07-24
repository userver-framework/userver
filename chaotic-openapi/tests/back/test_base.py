from chaotic_openapi.back.cpp_client import types


def test_base(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {},
    }
    assert translate_single_schema(schema) == types.ClientSpec(
        client_name='test',
        cpp_namespace='test_namespace',
        dynamic_config='',
    )
