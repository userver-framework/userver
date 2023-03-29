"""
Python plugin that provides gRPC fixtures for functional tests with
testsuite; see @ref md_en_userver_functional_testing for an introduction.

@ingroup userver_testsuite_fixtures
"""

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.grpc.client',
    'pytest_userver.plugins.grpc.mockserver',
]
