"""
Mocks for the gRPC servers, a.k.a. `pytest_userver.grpc.

@sa @ref scripts/docs/en/userver/tutorial/grpc_service.md
@sa @ref pytest_userver.plugins.grpc.mockserver
"""

from __future__ import annotations

import asyncio
import contextlib
import inspect
from typing import Any
from typing import Callable
from typing import Dict
from typing import Optional

import grpc

import testsuite.utils.callinfo

from ._mocked_errors import MockedError  # noqa: F401
from ._mocked_errors import NetworkError  # noqa: F401
from ._mocked_errors import TimeoutError  # noqa: F401
from ._servicer_mock import _check_is_servicer_class
from ._servicer_mock import _create_servicer_mock
from ._servicer_mock import _ServiceMock

Handler = Callable[[Any, grpc.aio.ServicerContext], Any]
MockDecorator = Callable[[Handler], testsuite.utils.callinfo.AsyncCallQueue]
AsyncExcAppend = Callable[[Exception], None]


class MockserverSession:
    """
    Allows to install mocks that are kept active between tests, see
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_session "grpc_mockserver_session".

    @warning This is a sharp knife, use with caution! For most use-cases, prefer
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver "grpc_mockserver" instead.
    """

    def __init__(self, *, server: grpc.aio.Server, experimental: bool = False) -> None:
        """
        @warning This initializer is an **experimental API**, likely to break in the future. Consider using
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_session "grpc_mockserver_session" instead.

        Initializes `MockserverSession`. Takes ownership of `server`.
        To properly start and stop the server, use `MockserverSession` as an async contextmanager:

        @code{.py}
        async with pytest_userver.grpc.mockserver.MockserverSession(server=server) as mockserver:
            # ...
        @endcode

        @note `MockserverSession` is usually obtained from
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_session "grpc_mockserver_session".
        """
        assert experimental
        self._server = server
        self._auto_service_mocks: Dict[type, _ServiceMock] = {}
        self._asyncexc_append: Optional[AsyncExcAppend] = None

    def _get_auto_service_mock(self, servicer_class: type, /) -> _ServiceMock:
        existing_mock = self._auto_service_mocks.get(servicer_class)
        if existing_mock:
            return existing_mock
        new_mock = _create_servicer_mock(servicer_class)
        new_mock.set_asyncexc_append(self._asyncexc_append)
        self._auto_service_mocks[servicer_class] = new_mock
        new_mock.add_to_server(self._server)
        return new_mock

    def _set_asyncexc_append(self, asyncexc_append: Optional[AsyncExcAppend], /) -> None:
        self._asyncexc_append = asyncexc_append
        for mock in self._auto_service_mocks.values():
            mock.set_asyncexc_append(asyncexc_append)

    def reset_mocks(self) -> None:
        """
        @brief Removes all mocks for this mockserver that have been installed using
        `MockserverSession` or @ref pytest_userver.grpc.Mockserver "Mockserver" API.
        @note Mocks installed manually using @ref MockserverSession.server will not be removed, though.
        """
        for mock in self._auto_service_mocks.values():
            mock.reset_handlers()

    @contextlib.contextmanager
    def asyncexc_append_scope(self, asyncexc_append: Optional[AsyncExcAppend], /):
        """
        Sets testsuite's `asyncexc_append` for use in the returned scope.
        """
        self._set_asyncexc_append(asyncexc_append)
        try:
            yield
        finally:
            self._set_asyncexc_append(None)

    @property
    def server(self) -> grpc.aio.Server:
        """
        The underlying [grpc.aio.Server](https://grpc.github.io/grpc/python/grpc_asyncio.html#grpc.aio.Server).
        """
        return self._server

    async def start(self) -> None:
        """
        Starts the server.
        @note Prefer starting mockserver using the async contextmanager syntax if possible.
        """
        await self._server.start()

    async def stop(self) -> None:
        """
        Stops the server properly. Prefer this method to stopping `server` manually.
        """
        await _stop_server(self._server)

    async def __aenter__(self) -> MockserverSession:
        await self.start()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb) -> None:
        await self.stop()


class Mockserver:
    """
    Allows to install mocks that are reset between tests, see
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver "grpc_mockserver".
    """

    def __init__(self, *, mockserver_session: MockserverSession, experimental: bool = False) -> None:
        """
        @warning This initializer is an **experimental API**, likely to break in the future. Consider using
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver "grpc_mockserver" instead.

        Initializes Mockserver.

        @note `Mockserver` is usually obtained from
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver "grpc_mockserver".
        """
        assert experimental
        self._mockserver_session = mockserver_session

    def __call__(self, servicer_method, /) -> MockDecorator:
        """
        Returns a decorator to mock the specified gRPC service method implementation.

        Example:

        @snippet samples/grpc_service/testsuite/test_grpc.py  Prepare modules
        @snippet samples/grpc_service/testsuite/test_grpc.py  grpc client test
        """
        servicer_class = _get_class_from_method(servicer_method)
        mock = self._mockserver_session._get_auto_service_mock(servicer_class)  # pylint: disable=protected-access
        return mock.install_handler(servicer_method.__name__)

    def mock_factory(self, servicer_class, /) -> Callable[[str], MockDecorator]:
        """
        Allows to create a fixture as a shorthand for mocking methods of the specified gRPC service.

        Example:

        @snippet grpc/functional_tests/metrics/tests/conftest.py  Prepare modules
        @snippet grpc/functional_tests/metrics/tests/conftest.py  Prepare server mock
        @snippet grpc/functional_tests/metrics/tests/test_metrics.py  grpc client test
        """

        def factory(method_name):
            method = getattr(servicer_class, method_name, None)
            if method is None:
                raise ValueError(f'No method "{method_name}" in servicer class "{servicer_class.__name__}"')
            return self(method)

        _check_is_servicer_class(servicer_class)
        return factory

    def install_servicer(self, servicer: object, /) -> None:
        """
        Installs as a mock `servicer`, the class of which should inherit from a generated `*Servicer` class.

        For example, @ref grpc/functional_tests/basic_chaos/tests-grpcclient/service.py "this servicer class"
        can be installed as follows:

        @snippet grpc/functional_tests/basic_chaos/tests-grpcclient/conftest.py installing mockserver servicer

        @note Inheritance from multiple `*Servicer` classes at once is allowed.

        @example grpc/functional_tests/basic_chaos/tests-grpcclient/service.py
        """
        base_servicer_classes = [cls for cls in inspect.getmro(type(servicer))[1:] if cls.__name__.endswith('Servicer')]
        if not base_servicer_classes:
            raise ValueError(f"Given object's type ({type(servicer)}) is not inherited from any grpc *Servicer class")
        for servicer_class in base_servicer_classes:
            # pylint: disable=protected-access
            mock = self._mockserver_session._get_auto_service_mock(servicer_class)
            for python_method_name in mock.known_methods:
                handler_func = getattr(servicer, python_method_name)
                mock.install_handler(python_method_name)(handler_func)


# @cond


async def _stop_server(server: grpc.aio.Server, /) -> None:
    async def stop_server():
        await server.stop(grace=None)
        await server.wait_for_termination()

    stop_server_task = asyncio.shield(asyncio.create_task(stop_server()))
    # asyncio.shield does not protect our await from cancellations, and we
    # really need to wait for the server stopping before continuing.
    try:
        await stop_server_task
    except asyncio.CancelledError:
        await stop_server_task
        # Propagate cancellation when we are done
        raise


def _get_class_from_method(method) -> type:
    # https://stackoverflow.com/a/54597033/5173839
    assert inspect.isfunction(method), f'Expected an unbound method: foo(ClassName.MethodName), got: {method}'
    class_name = method.__qualname__.split('.<locals>', 1)[0].rsplit('.', 1)[0]
    try:
        cls = getattr(inspect.getmodule(method), class_name)
    except AttributeError:
        cls = method.__globals__.get(class_name)
    assert isinstance(cls, type)
    return cls


# @endcond
