# type: ignore
"""Helpers to act as a minimal MD5 Digest client or server from Python."""

import hashlib
import re

from requests.auth import HTTPDigestAuth

# Parses both `key="value"` and `key=value` forms of digest directives.
_DIRECTIVE_RE = re.compile(r'(\w+)=("[^"]*"|[^,\s]+)')


def _md5(data: str) -> str:
    return hashlib.md5(data.encode('utf-8')).hexdigest()


def parse_directives(header: str) -> dict:
    """Parses a WWW-Authenticate or an Authorization header value."""
    prefix, _, params = header.partition(' ')
    assert prefix == 'Digest', header
    return {key: value.strip('"') for key, value in _DIRECTIVE_RE.findall(params)}


def construct_challenge(*, auth_directives: dict, nonce: str | None = None) -> dict:
    return {
        'realm': auth_directives['realm'],
        'nonce': nonce or auth_directives['nonce'],
        'algorithm': auth_directives['algorithm'],
        'qop': 'auth',
    }


class DigestAuthClient:
    """Minimal stateful Digest client for functional tests."""

    def __init__(self, *, username: str, password: str):
        self._digest_auth = HTTPDigestAuth(username, password)
        self._digest_auth.init_per_thread_state()

    def construct_header(self, *, challenge: dict, uri: str, method: str) -> str:
        """Builds an `Authorization: Digest ...` header value for the challenge."""
        # pylint: disable=protected-access
        self._digest_auth._thread_local.chal = challenge
        return self._digest_auth.build_digest_header(method, uri)


def construct_header(*, username: str, password: str, challenge: dict, uri: str, method: str) -> str:
    """Builds a single `Authorization: Digest ...` header value."""
    return DigestAuthClient(username=username, password=password).construct_header(
        challenge=challenge,
        uri=uri,
        method=method,
    )


def calculate_response(
    *,
    username: str,
    password: str,
    realm: str,
    nonce: str,
    nonce_count: str,
    cnonce: str,
    qop: str,
    method: str,
    uri: str,
) -> str:
    """Server side counterpart: the expected value of the `response` directive."""
    ha1 = _md5(f'{username}:{realm}:{password}')
    ha2 = _md5(f'{method}:{uri}')
    return _md5(f'{ha1}:{nonce}:{nonce_count}:{cnonce}:{qop}:{ha2}')
