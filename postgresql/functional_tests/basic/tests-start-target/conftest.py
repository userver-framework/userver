# Only the core plugin (plus the shared start-target plugin): the database is
# managed by the inner service-runner-mode session, so we must not load the
# postgresql plugin (which would try to bring up another database in this outer
# session).
pytest_plugins = [
    'pytest_userver.plugins.core',
    'start_target.pytest_plugin',
]
