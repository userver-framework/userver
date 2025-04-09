import pytest


@pytest.fixture(scope='session')
def _service_config_substitution_vars():
    return {
        'mockserver': '127.0.0.1:1234',
        'foo': 'foo_value',
    }


def test_simple(userver_config_substitutions):
    config_vars = {'simple': '$foo'}

    userver_config_substitutions({}, config_vars)

    assert config_vars == {'simple': 'foo_value'}


def test_substring(userver_config_substitutions):
    config_vars = {
        'frobnicator-endpoint': '$mockserver/some/path',
        'something': 'hmm&$foo-what',
    }

    userver_config_substitutions({}, config_vars)

    assert config_vars == {
        'frobnicator-endpoint': '127.0.0.1:1234/some/path',
        'something': 'hmm&foo_value-what',
    }


def test_substitute_nested(userver_config_substitutions):
    config_vars = {
        'list': ['what', '$foo', '$foo'],
        'dict': {'something': 10, 'what': '$foo'},
        'nested-list-of': [
            {
                'dicts-of': {
                    'lists': [
                        'what',
                        '$foo',
                    ],
                    'and-strings': '$foo',
                }
            }
        ],
    }

    userver_config_substitutions({}, config_vars)

    assert config_vars == {
        'list': ['what', 'foo_value', 'foo_value'],
        'dict': {'something': 10, 'what': 'foo_value'},
        'nested-list-of': [
            {
                'dicts-of': {
                    'lists': [
                        'what',
                        'foo_value',
                    ],
                    'and-strings': 'foo_value',
                }
            }
        ],
    }
