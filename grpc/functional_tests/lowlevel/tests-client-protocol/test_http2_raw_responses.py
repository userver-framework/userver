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

# RST_STREAM error code mapping https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html (HTTP2 Transport Mapping)
RST_ERROR_CODE_TO_STATUS = {
    http2.errors.ErrorCodes.NO_ERROR: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.PROTOCOL_ERROR: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.INTERNAL_ERROR: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.FLOW_CONTROL_ERROR: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.SETTINGS_TIMEOUT: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.FRAME_SIZE_ERROR: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.REFUSED_STREAM: grpc.StatusCode.UNAVAILABLE,  # Indicates that no processing occurred and the request can be retried, possibly elsewhere.
    # Mapped to call cancellation when sent by a client.Mapped to CANCELLED when sent by a server.
    # Note that servers should only use this mechanism when they need to cancel a call but the payload byte sequence is incomplete.
    http2.errors.ErrorCodes.CANCEL: grpc.StatusCode.CANCELLED,
    http2.errors.ErrorCodes.COMPRESSION_ERROR: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.CONNECT_ERROR: grpc.StatusCode.INTERNAL,
    http2.errors.ErrorCodes.ENHANCE_YOUR_CALM: grpc.StatusCode.RESOURCE_EXHAUSTED,  # with additional error detail provided by runtime to indicate that the exhausted resource is bandwidth.
    http2.errors.ErrorCodes.INADEQUATE_SECURITY: grpc.StatusCode.PERMISSION_DENIED,
}

HTTP_STATUSES = [
    *list(HTTP2_TO_GRPC),
    # not mapped statuses
    # TODO: think about 1xx, because their handling differs from other codes
    201,
    303,
    405,
    501,
]

# Codes from client settings in static config
RETRYABLE_STATUS_CODES = [
    grpc.StatusCode.UNAVAILABLE,
    grpc.StatusCode.ABORTED,
    grpc.StatusCode.CANCELLED,
    grpc.StatusCode.DEADLINE_EXCEEDED,
    grpc.StatusCode.INTERNAL,
]

RETRYABLE_HTTP_STATUSES = sorted(
    {status for status, code in HTTP2_TO_GRPC.items() if code in RETRYABLE_STATUS_CODES},
)

# Length-Prefixed-Message: Compressed-Flag (1 byte) Message-Length (4 bytes) Message
EMPTY_PROTO_MESSAGE = b'\x00' + b'\x00\x00\x00\x00'


async def test_success_response(service_client, grpc_server: http2.GrpcServer) -> None:
    def _response_factory() -> list[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Basic response is:
        # - Response-Headers (HEADERS frame): HTTP-status Content-Type
        # - Length-Prefixed-Message (DATA frame)
        # - Trailers (HEADERS frame, END_STREAM): Status

        response_headers = http2.HeadersFrame([
            (':status', '200'),
            ('content-type', 'application/grpc'),
        ])
        message = http2.DataFrame(EMPTY_PROTO_MESSAGE)
        trailers = http2.HeadersFrame([('grpc-status', utils.status_to_str(grpc.StatusCode.OK))], end_stream=True)

        return [response_headers, message, trailers]

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = grpc.StatusCode.OK
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code


@pytest.mark.parametrize('http_status', HTTP_STATUSES)
@pytest.mark.parametrize(
    'grpc_status_code',
    [grpc.StatusCode.OK, grpc.StatusCode.ABORTED, grpc.StatusCode.INTERNAL, grpc.StatusCode.UNAVAILABLE],
)
async def test_grpc_client_ignores_any_http_status_when_grpc_status_exists(
    service_client,
    grpc_server: http2.GrpcServer,
    http_status: int,
    grpc_status_code: grpc.StatusCode,
) -> None:
    def _response_factory() -> list[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        headers: list[tuple[str, str]] = [
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
    service_client,
    grpc_server: http2.GrpcServer,
    http_status: int,
) -> None:
    def _response_factory() -> list[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        headers: list[tuple[str, str]] = [
            (':status', str(http_status)),
            ('content-type', 'application/grpc'),
        ]

        return [http2.HeadersFrame(headers=headers, end_stream=True)]

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = http2_status_to_grpc(http_status)
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code


@pytest.mark.parametrize('grpc_status_code', RETRYABLE_STATUS_CODES)
async def test_grpc_client_retries_trailers_only_with_grpc_status(
    service_client,
    grpc_server: http2.GrpcServer,
    grpc_status_code: grpc.StatusCode,
    max_attempts: int,  # max attempts from service config
) -> None:
    attempts = 0

    def _response_factory() -> list[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        nonlocal attempts
        attempts += 1

        headers: list[tuple[str, str]] = [
            (':status', '200'),
            ('content-type', 'application/grpc'),
            ('grpc-status', utils.status_to_str(grpc_status_code)),
        ]

        return [http2.HeadersFrame(headers=headers, end_stream=True)]

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = grpc_status_code
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code
    assert attempts == max_attempts


@pytest.mark.parametrize('http_status', RETRYABLE_HTTP_STATUSES)
async def test_grpc_client_retries_trailers_only_with_retryable_http_status(
    service_client,
    grpc_server: http2.GrpcServer,
    http_status: int,
    max_attempts: int,  # max attempts from service config
) -> None:
    attempts = 0

    def _response_factory() -> list[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        nonlocal attempts
        attempts += 1

        headers: list[tuple[str, str]] = [
            (':status', str(http_status)),
            ('content-type', 'application/grpc'),
        ]

        return [http2.HeadersFrame(headers=headers, end_stream=True)]

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = http2_status_to_grpc(http_status)
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code
    assert attempts == max_attempts


@pytest.mark.parametrize('rst_stream_error_code', RST_ERROR_CODE_TO_STATUS)
@pytest.mark.parametrize('send_response_headers', [False, True])
async def test_grpc_client_converts_rst_stream_to_grpc_status(
    service_client,
    grpc_server: http2.GrpcServer,
    rst_stream_error_code: http2.errors.ErrorCodes,
    send_response_headers: bool,
) -> None:
    def _response_factory() -> list[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        response: list[http2.Frame] = []
        if send_response_headers:
            response.append(
                http2.HeadersFrame([
                    (':status', '200'),
                    ('content-type', 'application/grpc'),
                ]),
            )
        response.append(http2.RstStreamFrame(rst_stream_error_code))

        return response

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = RST_ERROR_CODE_TO_STATUS[rst_stream_error_code]
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code


async def test_grpc_client_retries_rst_stream_refused_stream(
    service_client,
    grpc_server: http2.GrpcServer,
    max_attempts: int,  # max attempts from service config
) -> None:
    # old gRPC version does not retry RST_STREAM(REFUSED_STREAM)
    if await client_grpc_version(service_client) < 1.54:
        pytest.skip('Does not work on old gRPC versions')

    attempts = 0

    def _response_factory() -> list[http2.Frame]:
        # https://grpc.github.io/grpc/core/md_doc__p_r_o_t_o_c_o_l-_h_t_t_p2.html
        # Trailers-Only is only HEADERS http2 frame (with END_STREAM flag)

        nonlocal attempts
        attempts += 1

        return [http2.RstStreamFrame(http2.errors.ErrorCodes.REFUSED_STREAM)]

    grpc_server.response_factory = _response_factory

    expected_grpc_status_code = RST_ERROR_CODE_TO_STATUS[http2.errors.ErrorCodes.REFUSED_STREAM]
    actual_grpc_status_code = await run_client(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code

    assert attempts == max_attempts


@pytest.mark.parametrize('invalid_grpc_status', [17, 100, 255, 1000, 100000])
async def test_grpc_client_converts_invalid_status_to_unknown(
    service_client,
    grpc_server: http2.GrpcServer,
    invalid_grpc_status: int,
) -> None:
    expected_grpc_status_code = grpc.StatusCode.UNKNOWN
    expected_grpc_status_details = 'nonstandarderror'

    support_non_standard_codes = await client_supports_non_standard_codes(service_client)

    if not support_non_standard_codes:
        pytest.skip('UBSan enabled')

    def _response_factory() -> list[http2.Frame]:
        # Send a response with an invalid grpc-status code
        headers: list[tuple[str, str]] = [
            (':status', '200'),
            ('content-type', 'application/grpc'),
            ('grpc-status', str(invalid_grpc_status)),
            ('grpc-message', expected_grpc_status_details),
        ]

        return [http2.HeadersFrame(headers=headers, end_stream=True)]

    grpc_server.response_factory = _response_factory

    actual_grpc_status_code, actual_grpc_status_details = await run_client_full_status(service_client)

    assert actual_grpc_status_code == expected_grpc_status_code, (
        'Expected grpc-status code to be UNKNOWN. userver must clamp out-of-range codes to UNKNOWN'
    )
    assert actual_grpc_status_details == expected_grpc_status_details


async def run_client(service_client) -> grpc.StatusCode:
    resp = await service_client.post('/client')
    assert resp.status == 200

    return utils.status_from_str(resp.json()['grpc-status'])


async def run_client_full_status(service_client) -> tuple[grpc.StatusCode, str]:
    resp = await service_client.post('/client')
    assert resp.status == 200

    return (utils.status_from_str(resp.json()['grpc-status']), resp.json()['grpc-message'])


async def client_grpc_version(service_client) -> float:
    resp = await service_client.get('/client')
    assert resp.status == 200

    version = resp.json()['grpc-version']

    return float(version['major']) + 0.01 * float(version['minor'])


async def client_supports_non_standard_codes(service_client) -> bool:
    resp = await service_client.get('/client')
    assert resp.status == 200

    return resp.json()['supports-non-standard-codes']


def http2_status_to_grpc(http_status: int) -> grpc.StatusCode:
    return HTTP2_TO_GRPC.get(http_status, grpc.StatusCode.UNKNOWN)
