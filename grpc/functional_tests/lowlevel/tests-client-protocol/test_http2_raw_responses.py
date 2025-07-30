from typing import List
from typing import Tuple

import grpc
import http2
import pytest

import utils

# mapped statuses according to https://grpc.github.io/grpc/cpp/md_doc_http-grpc-status-mapping.html
HTTP2_TO_GRPC = {
    400: grpc.StatusCode.INTERNAL,
    401: grpc.StatusCode.UNAUTHENTICATED,
    403: grpc.StatusCode.PERMISSION_DENIED,
    404: grpc.StatusCode.UNIMPLEMENTED,
    429: grpc.StatusCode.UNAVAILABLE,
    502: grpc.StatusCode.UNAVAILABLE,
    503: grpc.StatusCode.UNAVAILABLE,
    504: grpc.StatusCode.UNAVAILABLE,
}

HTTP_STATUSES = list(HTTP2_TO_GRPC) + [
    # not mapped statuses
    # TODO: think about 1xx, because their handling differs from other codes
    201,
    303,
    405,
    501,
]


@pytest.mark.parametrize('http_status', HTTP_STATUSES)
@pytest.mark.parametrize(
    'grpc_status_code',
    [grpc.StatusCode.OK, grpc.StatusCode.ABORTED, grpc.StatusCode.INTERNAL, grpc.StatusCode.UNAVAILABLE],
)
async def test_grpc_client_ignores_any_http_status_when_grpc_status_exists(
    service_client,
    http_status: int,
    grpc_status_code: grpc.StatusCode,
    grpc_server: http2.GrpcServer,
) -> None:
    def _response_factory() -> List[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        headers: List[Tuple[str, str]] = [
            (':status', str(http_status)),
            ('content-type', 'application/grpc'),
            ('grpc-status', utils.status_to_str(grpc_status_code)),
        ]

        return [http2.HeadersFrame(headers=headers, end_stream=True)]

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = grpc_status_code
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code


@pytest.mark.parametrize('http_status', HTTP_STATUSES)
async def test_grpc_client_synthesizes_grpc_status_from_http_status(
    service_client, http_status: int, grpc_server: http2.GrpcServer
) -> None:
    def _response_factory() -> List[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        headers: List[Tuple[str, str]] = [
            (':status', str(http_status)),
            ('content-type', 'application/grpc'),
        ]

        return [http2.HeadersFrame(headers=headers, end_stream=True)]

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = http2_status_to_grpc(http_status)
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code


async def run_client(service_client) -> grpc.StatusCode:
    resp = await service_client.post('/client')
    assert resp.status == 200

    return utils.status_from_str(resp.json()['grpc-status'])


def http2_status_to_grpc(http_status: int) -> grpc.StatusCode:
    return HTTP2_TO_GRPC.get(http_status, grpc.StatusCode.UNKNOWN)
