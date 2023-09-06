# type: ignore
import re

from requests.auth import HTTPDigestAuth


# parsing regex
REG = re.compile(r'(\w+)[:=][\s"]?([^",]+)"?')


# construct challenge structer
def construct_challenge(auth_directives: dict, nonce=''):
    return {
        'realm': auth_directives['realm'],
        'nonce': auth_directives['nonce'] if len(nonce) == 0 else nonce,
        'algorithm': auth_directives['algorithm'],
        'qop': 'auth',
    }


# construct authorization header sent from client
def construct_header(username: str, password: str, challenge: dict):
    digest_auth = HTTPDigestAuth(username, password)
    digest_auth.init_per_thread_state()
    # pylint: disable=protected-access
    digest_auth._thread_local.chal = challenge
    return digest_auth.build_digest_header('GET', '/v1/hello')


def parse_directives(authentication_header: str):
    return dict(REG.findall(authentication_header))


# checks if mandatory directives are in WWW-Authenticate header
def auth_directives_assert(auth_directives: dict):
    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives
