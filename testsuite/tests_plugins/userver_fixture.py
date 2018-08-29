import collections
import os.path
import pytest
import uuid

from taxi_tests.daemons import service_client
from taxi_tests.daemons import service_daemon

from tests_plugins import userver_main


class DaemonSettings(service_daemon.ServiceDaemonSettings):
    pass


# TODO: fetch port from userver service config file
USERVER_BASEURL = 'http://localhost:1180/'
UServerServiceConfig = collections.namedtuple(
    'UServerServiceConfig', ['binary', 'config']

)


def create_client_fixture(service_name, scope='session', daemon_deps=(),
                          client_deps=(),
                          client_class=None,
                          service_headers={}):
    """Creates userver daemon fixture.

    Returns client fixture and installs daemon fixture into module referenced
    by modname.

    :param modname: module name to inject fixtures into
    :param scope: daemon fixture scope, session by default
    :param daemon_deps: daemon fixture dependencies
    :param client_deps: client fixture dependencies
    :return: client fixture
    """

    name = _normalize_name(service_name)
    module = userver_main

    if client_class is None:
        client_class = service_client.ServiceClientTestsControl

    @pytest.yield_fixture(scope=scope)
    def daemon_fixture(register_daemon_scope, settings, request):
        for dep in daemon_deps:
            request.getfuncargvalue(dep)

        service = _service_config(service_name, settings)
        args = [service.binary, '-c', service.config]

        with register_daemon_scope(
                name=service_name,
                spawn=lambda: service_daemon.start(
                    args, check_url=USERVER_BASEURL + 'ping',
                    settings=getattr(settings, 'userver', None),
                )
        ) as scope:
            yield scope
    daemon_fixture.__name__ = '_%s_daemon_%s' % (
        _normalize_name(name), uuid.uuid4().hex
    )

    @pytest.fixture
    def client_fixture(request, load_fixture, translations, settings, now,
                       ensure_daemon_started):
        for dep in client_deps:
            request.getfuncargvalue(dep)

        daemon = request.getfuncargvalue(daemon_fixture.__name__)
        ensure_daemon_started(daemon)
        client = client_class(USERVER_BASEURL, now=now,
                              service_headers=service_headers)
        return client

    setattr(module, daemon_fixture.__name__, daemon_fixture)
    return client_fixture


_service_config_cache = {}


def _service_config(service_name, settings):
    if service_name not in _service_config_cache:
        service_path = os.path.abspath(
            os.path.join(
                settings.TAXI_BUILD_DIR,
                'testsuite/services/%s.service' % (service_name,)
            )
        )
        service_vars = {}
        with open(service_path) as fp:
            for line in fp:
                key, value = line.strip().split('=', 1)
                service_vars[key] = value

        assert 'BINARY' in service_vars
        assert 'CONFIG' in service_vars

        _service_config_cache[service_name] = UServerServiceConfig(
            binary=service_vars['BINARY'],
            config=service_vars['CONFIG'],
        )
    return _service_config_cache[service_name]


def _normalize_name(name):
    return name.replace('-', '_').replace('.', '_')
