import pytest

from chaotic import error


@pytest.mark.parametrize('version', [{'swagger': '2.0'}, {'openapi': '3.0.0'}])
@pytest.mark.parametrize('extension', ['x-usrv-expected-status-codes', 'x-taxi-expected-status-codes'])
def test_expected_status_codes_inheritance(simple_parser, version, extension):
    service = simple_parser({
        **version,
        extension: [404, 409],
        'paths': {
            '/inherited': {'get': {'responses': {}}},
            '/overridden': {'get': {'responses': {}, extension: [403]}},
            '/cleared': {'get': {'responses': {}, extension: []}},
        },
    })
    assert [op.expected_status_codes for op in service.operations] == [[404, 409], [403], []]


@pytest.mark.parametrize('version', [{'swagger': '2.0'}, {'openapi': '3.0.0'}])
@pytest.mark.parametrize('codes', [None, '404', [True], ['404'], [99], [600], [404.0]])
@pytest.mark.parametrize('level', ['client', 'method'])
def test_invalid_expected_status_codes(simple_parser, version, codes, level):
    operation = {'responses': {}}
    schema = {**version, 'paths': {'/test': {'get': operation}}}
    target = schema if level == 'client' else operation
    target['x-usrv-expected-status-codes'] = codes
    with pytest.raises(error.BaseError):
        simple_parser(schema)


def test_responses_do_not_imply_expected_status_codes(simple_parser):
    service = simple_parser({
        'openapi': '3.0.0',
        'paths': {'/test': {'get': {'responses': {'404': {'description': 'Not found'}}}}},
    })
    assert service.operations[0].expected_status_codes is None
