import functional_tests.error_status_pb2 as error_status_protos
import grpc
import pytest


@pytest.mark.servicetest
async def test_ping(grpc_client):
    request = error_status_protos.ErrorStatusRequest(status_code=grpc.StatusCode.OK.name, status_message='')
    await grpc_client.ReturnErrorStatus(request)


async def test_error_status(grpc_client):
    for status_code in grpc.StatusCode:
        if status_code == grpc.StatusCode.OK:
            continue
        request = error_status_protos.ErrorStatusRequest(status_code=status_code.name, status_message='')
        with pytest.raises(grpc.RpcError) as exc_info:
            await grpc_client.ReturnErrorStatus(request)
        assert exc_info.value.code() == status_code


@pytest.mark.parametrize(
    'status_code,request_message,response_message',
    [
        (grpc.StatusCode.PERMISSION_DENIED, 'Permission denied', 'Permission denied'),  # do not trim
        (grpc.StatusCode.INVALID_ARGUMENT, 'Invalid argument ', 'Invalid argument'),  # trim at the end
        (grpc.StatusCode.NOT_FOUND, ' Not found', 'Not found'),  # trim at the beginning
        (grpc.StatusCode.CANCELLED, ' Cancelled  ', 'Cancelled'),  # trim at the beginning and at the end
        (grpc.StatusCode.UNIMPLEMENTED, 'Не реализовано', 'Не реализовано'),  # do not trim utf-8
        (grpc.StatusCode.OUT_OF_RANGE, '  Нарушение границ', 'Нарушение границ'),  # utf-8 trim at the beginning
        (grpc.StatusCode.ABORTED, 'Аварийной завершение  ', 'Аварийной завершение'),  # utf-8 trim at the end
        (grpc.StatusCode.UNKNOWN, ' Неизвестная ошибка ', 'Неизвестная ошибка'),  # trim at the beginning and at the end
    ],
)
async def test_error_message_trim(grpc_client, status_code, request_message, response_message):
    request = error_status_protos.ErrorStatusRequest(status_code=status_code.name, status_message=request_message)

    methods = [grpc_client.ReturnErrorStatus, grpc_client.ThrowErrorWithStatusException]

    for method in methods:
        with pytest.raises(grpc.aio.AioRpcError) as exc_info:
            await method(request)

        assert exc_info.value.code() == status_code
        assert exc_info.value.details() == response_message

    methods = [grpc_client.ReturnStreamErrorStatus, grpc_client.ThrowStreamErrorWithStatusException]

    for stream_method in methods:
        response_count = 0
        with pytest.raises(grpc.aio.AioRpcError) as exc_info:
            async for response in stream_method(request):
                response_count += 1

        assert response_count > 0
        assert exc_info.value.code() == status_code
        assert exc_info.value.details() == response_message


async def test_runtime_error(grpc_client):
    request = error_status_protos.RuntimeErrorRequest(message=' Runtime ошибка ')

    with pytest.raises(grpc.aio.AioRpcError) as exc_info:
        await grpc_client.ThrowRuntimeErrorException(request)

    assert exc_info.value.code() == grpc.StatusCode.UNKNOWN
    assert not exc_info.value.details().startswith(' ')
    assert not exc_info.value.details().endswith(' ')
