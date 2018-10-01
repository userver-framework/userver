pytest_plugins = [
    'taxi_tests.daemons.plugins',
    'taxi_tests.environment.pytest_plugin',
    'taxi_tests.plugins.default',
    'taxi_tests.plugins.aliases',
    'taxi_tests.plugins.translations',
    'tests_plugins.userver_main',
]
