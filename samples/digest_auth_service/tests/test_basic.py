import pytest
import requests.auth
from requests.auth import HTTPDigestAuth
import re
import hashlib
import time

reg=re.compile('(\w+)[:=][\s"]?([^",]+)"?')

@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_authenticate_base(service_client):
    ## initial request without Authorization
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    ## parse WWW-Authenticate header into dictionary of directives and values
    authentication_header = response.headers["WWW-Authenticate"]
    authentication_directives = dict(reg.findall(authentication_header))

    assert 'realm' in authentication_directives
    assert 'nonce' in authentication_directives
    assert 'algorithm' in authentication_directives
    assert 'qop' in authentication_directives

    ## now construct Authorization header sent from client
    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives["nonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    ## response will be calculated below
    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    ## now send request with constructed Authorization header
    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_postgres_wrong_data(service_client):
    ## initial request without Authorization
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    ## we need to repeat request:
    authentication_header = response.headers["WWW-Authenticate"]
    authentication_directives = dict(reg.findall(authentication_header))

    assert 'realm' in authentication_directives
    assert 'nonce' in authentication_directives
    assert 'algorithm' in authentication_directives
    assert 'qop' in authentication_directives

    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives["nonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    a = HTTPDigestAuth("username", "WRONG-PASSWORD")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 401


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_repeated_auth(service_client):
    ## initial request without Authorization
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    ## parse WWW-Authenticate header into dictionary of directives and values
    authentication_header = response.headers["WWW-Authenticate"]
    authentication_directives = dict(reg.findall(authentication_header))

    assert 'realm' in authentication_directives
    assert 'nonce' in authentication_directives
    assert 'algorithm' in authentication_directives
    assert 'qop' in authentication_directives

    ## now construct Authorization header sent from client
    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives["nonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    ## response will be calculated below
    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    ## now send request with constructed Authorization header
    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    ## we need to repeat request:
    authentication_info_header = response.headers["Authentication-Info"]
    authentication_directives_info = dict(reg.findall(authentication_info_header))

    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives_info["nextnonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200 ## success


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_same_nonce_repeat_use(service_client):
    ## initial request without Authorization
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    ## parse WWW-Authenticate header into dictionary of directives and values
    authentication_header = response.headers["WWW-Authenticate"]
    authentication_directives = dict(reg.findall(authentication_header))

    assert 'realm' in authentication_directives
    assert 'nonce' in authentication_directives
    assert 'algorithm' in authentication_directives
    assert 'qop' in authentication_directives

    ## now construct Authorization header sent from client
    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives["nonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    ## response will be calculated below
    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    ## now send request with constructed Authorization header
    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    ## we need to repeat request:
    authentication_info_header = response.headers["Authentication-Info"]
    authentication_directives_info = dict(reg.findall(authentication_info_header))

    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives_info["nextnonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200 ## success

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )

    assert response.status == 401


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_expiring_nonce(service_client, mocked_time):
    ## initial request without Authorization
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    ## parse WWW-Authenticate header into dictionary of directives and values
    authentication_header = response.headers["WWW-Authenticate"]
    authentication_directives = dict(reg.findall(authentication_header))

    assert 'realm' in authentication_directives
    assert 'nonce' in authentication_directives
    assert 'algorithm' in authentication_directives
    assert 'qop' in authentication_directives

    ## now construct Authorization header sent from client
    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives["nonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    ## response will be calculated below
    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    ## now send request with constructed Authorization header
    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    mocked_time.sleep(1500)

    authentication_info_header = response.headers["Authentication-Info"]
    authentication_directives_info = dict(reg.findall(authentication_info_header))

    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives_info["nextnonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 401

    ## parse WWW-Authenticate header into dictionary of directives and values
    authentication_header = response.headers["WWW-Authenticate"]
    authentication_directives = dict(reg.findall(authentication_header))

    assert 'realm' in authentication_directives
    assert 'nonce' in authentication_directives
    assert 'algorithm' in authentication_directives
    assert 'qop' in authentication_directives

    ## now construct Authorization header sent from client
    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives["nonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    ## response will be calculated below
    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    ## now send request with constructed Authorization header
    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )

    assert response.status == 200


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_aliving_nonce_after_half_ttl(service_client, mocked_time):
    ## initial request without Authorization
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    ## parse WWW-Authenticate header into dictionary of directives and values
    authentication_header = response.headers["WWW-Authenticate"]
    authentication_directives = dict(reg.findall(authentication_header))

    assert 'realm' in authentication_directives
    assert 'nonce' in authentication_directives
    assert 'algorithm' in authentication_directives
    assert 'qop' in authentication_directives

    ## now construct Authorization header sent from client
    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives["nonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    ## response will be calculated below
    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    ## now send request with constructed Authorization header
    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    mocked_time.sleep(500)

    authentication_info_header = response.headers["Authentication-Info"]
    authentication_directives_info = dict(reg.findall(authentication_info_header))

    chal = {'realm': authentication_directives["realm"], 
            'nonce': authentication_directives_info["nextnonce"],
            'algorithm': authentication_directives["algorithm"],
            'qop': "auth"
            }

    a = HTTPDigestAuth("username", "pswd")
    a.init_per_thread_state()
    a._thread_local.chal = chal
    auth_header = a.build_digest_header('GET', '/v1/hello')

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200
    