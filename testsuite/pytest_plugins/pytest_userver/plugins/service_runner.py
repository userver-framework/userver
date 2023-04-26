"""
Helpers to make the `make start-*` commands work.
"""

# pylint: disable=no-member,missing-kwoa
import pathlib

import py.path
import pytest


class ServiceRunnerModule(pytest.Module):
    class FakeModule:
        def __init__(self, fspath):
            self.__file__ = fspath

    def __init__(self, **kwargs) -> None:
        super().__init__(**kwargs)
        self._module = self.FakeModule(str(self.fspath))

    @property
    def obj(self):
        return self._module


class UserviceRunner:
    @pytest.hookimpl(tryfirst=True)
    def pytest_collection_modifyitems(self, session, config, items):
        paths = set()

        # Is there servicetest chosen
        for item in items:
            paths.add(pathlib.Path(item.module.__file__).parent)
            for marker in item.own_markers:
                if marker.name == 'servicetest':
                    return

        if not paths:
            return

        tests_root = min(paths, key=lambda p: len(p.parts))

        module = ServiceRunnerModule.from_parent(
            parent=session, fspath=py.path.local(tests_root / '__service__'),
        )
        function = pytest.Function.from_parent(
            parent=module,
            name=test_service_default.__name__,
            callobj=test_service_default,
        )

        items.append(function)


@pytest.mark.servicetest
def test_service_default(
        service_client, service_baseurl, monitor_baseurl,
) -> None:
    """
    This is default service runner testcase. Feel free to override it
    in your own tests, e.g.:

    @code
    @pytest.mark.servicetest
    def test_service(service_client):
        ...
    @endcode

    @ingroup userver_testsuite
    """
    # TODO: use service_client.base_url() and monitor_client.base_url()
    delimiter = '=' * 100
    message = f'\n{delimiter}\nStarted service at {service_baseurl}'
    if monitor_baseurl:
        message += f', configured monitor URL is {monitor_baseurl}'
    message += f'\n{delimiter}\n'
    print(message)


def pytest_configure(config):
    if config.option.service_runner_mode:
        runner = UserviceRunner()
        config.pluginmanager.register(runner, 'uservice_runner')
