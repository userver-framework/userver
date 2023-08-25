from requests.auth import HTTPDigestAuth
import re
# parsing regex
reg = re.compile(r'(\w+)[:=][\s"]?([^",]+)"?')

# construct challenge structer
def construct_challenge(auth_directives: dict, nonce=''):
    return {'realm': auth_directives["realm"],
            'nonce': auth_directives['nonce'] if len(nonce) == 0 else nonce,
            'algorithm': auth_directives["algorithm"],
            'qop': "auth"
            }


# construct authorization header sent from client
def construct_header(username: str, password: str, challenge: dict):
    digest_auth = HTTPDigestAuth(username, password)
    digest_auth.init_per_thread_state()
    digest_auth._thread_local.chal = challenge
    return digest_auth.build_digest_header('GET', '/v1/hello')


def parse_directives(authentication_header: str):
    return dict(reg.findall(authentication_header))