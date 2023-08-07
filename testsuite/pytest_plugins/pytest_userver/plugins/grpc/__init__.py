"""
Python plugin that provides gRPC fixtures for functional tests with
testsuite; see
@ref scripts/docs/en/userver/functional_testing.md for an introduction.

@ingroup userver_testsuite_fixtures
"""

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.grpc.client',
    'pytest_userver.plugins.grpc.mockserver',
]
