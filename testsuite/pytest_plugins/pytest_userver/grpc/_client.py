import abc
from collections.abc import AsyncIterator
from collections.abc import Awaitable
from collections.abc import Callable
from typing import Any

import grpc.aio


class PreCallClientInterceptor(
    grpc.aio.UnaryUnaryClientInterceptor,
    grpc.aio.UnaryStreamClientInterceptor,
    grpc.aio.StreamUnaryClientInterceptor,
    grpc.aio.StreamStreamClientInterceptor,
    abc.ABC,
):
    """
    @brief A simple generic interceptor that allows to insert some code before each RPC is initiated.

    To use, inherit from this class and pass an instance to
    @ref pytest_userver.plugins.grpc._hookspec.pytest_grpc_client_interceptors "pytest_grpc_client_interceptors".

    @note Exported as `pytest_userver.grpc.PreCallClientInterceptor`.
    @ingroup userver_testsuite
    """

    @abc.abstractmethod
    async def pre_call_hook(self, client_call_details: grpc.aio.ClientCallDetails) -> None:
        """
        Runs before the RPC is initiated. `client_call_details` can be observed or modified as needed.
        """

    async def intercept_unary_unary(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, Any], Awaitable[grpc.aio.UnaryUnaryCall]],
        client_call_details: grpc.aio.ClientCallDetails,
        request: Any,
    ) -> grpc.aio.UnaryUnaryCall:
        await self.pre_call_hook(client_call_details)
        return await continuation(client_call_details, request)

    # Note: full type of this function is Callable[[...], Awaitable[AsyncIterator[Any]]]
    async def intercept_unary_stream(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, Any], grpc.aio.UnaryStreamCall],
        client_call_details: grpc.aio.ClientCallDetails,
        request: Any,
    ) -> AsyncIterator[Any]:
        await self.pre_call_hook(client_call_details)
        return await continuation(client_call_details, request)

    async def intercept_stream_unary(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, AsyncIterator[Any]], Awaitable[grpc.aio.StreamUnaryCall]],
        client_call_details: grpc.aio.ClientCallDetails,
        request_iterator: AsyncIterator[Any],
    ) -> grpc.aio.StreamUnaryCall:
        await self.pre_call_hook(client_call_details)
        return await continuation(client_call_details, request_iterator)

    # Note: full type of this function is Callable[[...], Awaitable[AsyncIterator[Any]]]
    async def intercept_stream_stream(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, AsyncIterator[Any]], grpc.aio.StreamStreamCall],
        client_call_details: grpc.aio.ClientCallDetails,
        request_iterator: AsyncIterator[Any],
    ) -> AsyncIterator[Any]:
        await self.pre_call_hook(client_call_details)
        return await continuation(client_call_details, request_iterator)
