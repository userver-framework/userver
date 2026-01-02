"""userver pytest hooks for testing gRPC clients and services."""

from collections.abc import Sequence

import grpc.aio
import pytest


@pytest.hookspec
def pytest_grpc_client_interceptors(request: pytest.FixtureRequest) -> Sequence[grpc.aio.ClientInterceptor]:
    """
    A pytest hook that returns gRPC client interceptors to use when making requests to the service.

    Interceptors are accomulated over all implementations of this hook from all plugins.

    @see @ref scripts/docs/en/userver/grpc/grpc.md
    @ingroup userver_testsuite_fixtures
    """
    raise NotImplementedError()
