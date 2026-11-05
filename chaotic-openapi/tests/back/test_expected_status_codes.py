from chaotic_openapi.back.cpp.client import renderer


def test_expected_status_codes_reach_generated_client(translate_single_schema):
    spec = translate_single_schema({
        'openapi': '3.0.0',
        'x-usrv-expected-status-codes': [404],
        'paths': {
            '/inherited': {'get': {'responses': {}}},
            '/cleared': {'get': {'responses': {}, 'x-usrv-expected-status-codes': []}},
        },
    })
    assert [op.expected_status_codes for op in spec.operations] == [[404], []]
    template = renderer.JINJA_ENV.get_template('templates/client_impl.cpp.jinja')
    result = template.render(
        spec=spec,
        namespace='test_namespace',
        name='test',
        config='',
        operations=spec.operations,
    )
    assert 'r.SetExpectedStatusCodes({ static_cast<USERVER_NAMESPACE::http::StatusCode>(404) });' in result
    assert 'r.SetExpectedStatusCodes({  });' in result
