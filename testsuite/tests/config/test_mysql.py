import copy
import json
from types import SimpleNamespace

import pytest
from pytest_userver.plugins import mysql


@pytest.mark.parametrize('hostname,password', [('localhost', None), ('127.0.0.1', 'test-password'), ('::1', None)])
def test_default_mysql_endpoint(hostname, password):
    conninfo = SimpleNamespace(hostname='db.example', port=23307, user='test-user', password=password)
    secdist = {
        'mysql_settings': {
            'sample': {
                'hosts': [hostname],
                'port': 13307,
                'user': 'root',
                'password': '',
                'database': 'sample_db',
            },
        },
        'other_settings': {'preserved': True},
    }

    assert mysql._patch_mysql_secdist(secdist, conninfo)

    assert secdist == {
        'mysql_settings': {
            'sample': {
                'hosts': ['db.example'],
                'port': 23307,
                'user': 'test-user',
                'password': password or '',
                'database': 'sample_db',
            },
        },
        'other_settings': {'preserved': True},
    }


@pytest.mark.parametrize(
    'settings',
    [
        {'hosts': ['localhost'], 'port': 19355},
        {'hosts': ['db.example'], 'port': 13307},
        {'hosts': ['localhost', 'replica.example'], 'port': 13307},
        {'hosts': ['localhost']},
    ],
)
def test_explicit_mysql_endpoint_is_preserved(settings):
    secdist = {'mysql_settings': {'custom': settings}}
    original = copy.deepcopy(secdist)
    conninfo = SimpleNamespace(hostname='localhost', port=23307, user='root', password=None)

    assert not mysql._patch_mysql_secdist(secdist, conninfo)
    assert secdist == original


def test_secdist_without_mysql():
    secdist = {'other_settings': {'preserved': True}}
    original = copy.deepcopy(secdist)

    assert not mysql._patch_mysql_secdist(secdist, None)
    assert secdist == original


@pytest.fixture(scope='session')
def mysql_conninfo():
    return SimpleNamespace(hostname='127.0.0.1', port=23307, user='test-user', password='test-password')


@pytest.fixture(scope='session')
def service_env():
    return {}


@pytest.mark.parametrize('env_key', ['SECDIST_CONFIG', 'CUSTOM_SECDIST'])
def test_mysql_config_hook(userver_config_mysql, service_env, mysql_conninfo, monkeypatch, env_key):
    config = {
        'components_manager': {
            'components': {'default-secdist-provider': {'environment-secrets-key': env_key}},
        },
    }
    secdist = {'mysql_settings': {'sample': {'hosts': ['localhost'], 'port': 13307, 'database': 'sample_db'}}}
    monkeypatch.setitem(service_env, env_key, json.dumps(secdist))

    userver_config_mysql(config, {})

    settings = json.loads(service_env[env_key])['mysql_settings']['sample']
    assert settings == {
        'hosts': [mysql_conninfo.hostname],
        'port': mysql_conninfo.port,
        'user': mysql_conninfo.user,
        'password': mysql_conninfo.password,
        'database': 'sample_db',
    }


@pytest.mark.parametrize('provider', [None, {}, {'config': '/etc/secdist.json'}, {'environment-secrets-key': 'UNSET'}])
def test_mysql_config_hook_without_environment(userver_config_mysql, service_env, provider):
    config = {'components_manager': {'components': {'default-secdist-provider': provider}}}
    original_config = copy.deepcopy(config)
    original_env = dict(service_env)

    userver_config_mysql(config, {})

    assert config == original_config
    assert service_env == original_env
