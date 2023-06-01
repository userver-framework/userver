"""
Python module that provides clients for functional tests with
testsuite; see @ref md_en_userver_functional_testing for an introduction.

@ingroup userver_testsuite
"""

import contextlib
import copy
import dataclasses
import json
import logging
import typing

import aiohttp

from testsuite import annotations
from testsuite import utils
from testsuite.daemons import service_client
from testsuite.utils import approx
from testsuite.utils import http

import pytest_userver.metrics as metric_module  # pylint: disable=import-error
from pytest_userver.plugins import caches

# @cond
logger = logging.getLogger(__name__)
# @endcond

_UNKNOWN_STATE = '__UNKNOWN__'


class BaseError(Exception):
    """Base class for exceptions of this module."""


class TestsuiteActionFailed(BaseError):
    pass


class TestsuiteTaskError(BaseError):
    pass


class TestsuiteTaskNotFound(TestsuiteTaskError):
    pass


class TestsuiteTaskConflict(TestsuiteTaskError):
    pass


class ConfigurationError(BaseError):
    pass


class PeriodicTaskFailed(BaseError):
    pass


class PeriodicTasksState:
    def __init__(self):
        self.suspended_tasks: typing.Set[str] = set()
        self.tasks_to_suspend: typing.Set[str] = set()


class TestsuiteTaskFailed(TestsuiteTaskError):
    def __init__(self, name, reason):
        self.name = name
        self.reason = reason
        super().__init__(f'Testsuite task {name!r} failed: {reason}')


@dataclasses.dataclass(frozen=True)
class TestsuiteClientConfig:
    testsuite_action_path: typing.Optional[str] = None
    server_monitor_path: typing.Optional[str] = None


Metric = metric_module.Metric


class ClientWrapper:
    """
    Base asyncio userver client that implements HTTP requests to service.

    Compatible with werkzeug interface.

    @ingroup userver_testsuite
    """

    def __init__(self, client):
        self._client = client

    async def post(
            self,
            path: str,
            json: annotations.JsonAnyOptional = None,
            data: typing.Any = None,
            params: typing.Optional[typing.Dict[str, str]] = None,
            bearer: typing.Optional[str] = None,
            x_real_ip: typing.Optional[str] = None,
            headers: typing.Optional[typing.Dict[str, str]] = None,
            **kwargs,
    ) -> http.ClientResponse:
        """
        Make a HTTP POST request
        """
        response = await self._client.post(
            path,
            json=json,
            data=data,
            params=params,
            headers=headers,
            bearer=bearer,
            x_real_ip=x_real_ip,
            **kwargs,
        )
        return await self._wrap_client_response(response)

    async def put(
            self,
            path,
            json: annotations.JsonAnyOptional = None,
            data: typing.Any = None,
            params: typing.Optional[typing.Dict[str, str]] = None,
            bearer: typing.Optional[str] = None,
            x_real_ip: typing.Optional[str] = None,
            headers: typing.Optional[typing.Dict[str, str]] = None,
            **kwargs,
    ) -> http.ClientResponse:
        """
        Make a HTTP PUT request
        """
        response = await self._client.put(
            path,
            json=json,
            data=data,
            params=params,
            headers=headers,
            bearer=bearer,
            x_real_ip=x_real_ip,
            **kwargs,
        )
        return await self._wrap_client_response(response)

    async def patch(
            self,
            path,
            json: annotations.JsonAnyOptional = None,
            data: typing.Any = None,
            params: typing.Optional[typing.Dict[str, str]] = None,
            bearer: typing.Optional[str] = None,
            x_real_ip: typing.Optional[str] = None,
            headers: typing.Optional[typing.Dict[str, str]] = None,
            **kwargs,
    ) -> http.ClientResponse:
        """
        Make a HTTP PATCH request
        """
        response = await self._client.patch(
            path,
            json=json,
            data=data,
            params=params,
            headers=headers,
            bearer=bearer,
            x_real_ip=x_real_ip,
            **kwargs,
        )
        return await self._wrap_client_response(response)

    async def get(
            self,
            path: str,
            headers: typing.Optional[typing.Dict[str, str]] = None,
            bearer: typing.Optional[str] = None,
            x_real_ip: typing.Optional[str] = None,
            **kwargs,
    ) -> http.ClientResponse:
        """
        Make a HTTP GET request
        """
        response = await self._client.get(
            path,
            headers=headers,
            bearer=bearer,
            x_real_ip=x_real_ip,
            **kwargs,
        )
        return await self._wrap_client_response(response)

    async def delete(
            self,
            path: str,
            headers: typing.Optional[typing.Dict[str, str]] = None,
            bearer: typing.Optional[str] = None,
            x_real_ip: typing.Optional[str] = None,
            **kwargs,
    ) -> http.ClientResponse:
        """
        Make a HTTP DELETE request
        """
        response = await self._client.delete(
            path,
            headers=headers,
            bearer=bearer,
            x_real_ip=x_real_ip,
            **kwargs,
        )
        return await self._wrap_client_response(response)

    async def options(
            self,
            path: str,
            headers: typing.Optional[typing.Dict[str, str]] = None,
            bearer: typing.Optional[str] = None,
            x_real_ip: typing.Optional[str] = None,
            **kwargs,
    ) -> http.ClientResponse:
        """
        Make a HTTP OPTIONS request
        """
        response = await self._client.options(
            path,
            headers=headers,
            bearer=bearer,
            x_real_ip=x_real_ip,
            **kwargs,
        )
        return await self._wrap_client_response(response)

    async def request(
            self, http_method: str, path: str, **kwargs,
    ) -> http.ClientResponse:
        """
        Make a HTTP request with the specified method
        """
        response = await self._client.request(http_method, path, **kwargs)
        return await self._wrap_client_response(response)

    def _wrap_client_response(
            self, response: aiohttp.ClientResponse,
    ) -> typing.Awaitable[http.ClientResponse]:
        return http.wrap_client_response(response)


# @cond


def _wrap_client_error(func):
    async def _wrapper(*args, **kwargs):
        try:
            return await func(*args, **kwargs)
        except aiohttp.client_exceptions.ClientResponseError as exc:
            raise http.HttpResponseError(
                url=exc.request_info.url, status=exc.status,
            )

    return _wrapper


class AiohttpClientMonitor(service_client.AiohttpClient):
    _config: TestsuiteClientConfig

    def __init__(self, base_url, *, config: TestsuiteClientConfig, **kwargs):
        super().__init__(base_url, **kwargs)
        self._config = config

    async def get_metrics(self, prefix=None):
        if not self._config.server_monitor_path:
            raise ConfigurationError(
                'handler-server-monitor component is not configured',
            )
        if prefix is not None:
            params = {'prefix': prefix}
        else:
            params = None
        response = await self.get(
            self._config.server_monitor_path, params=params,
        )
        async with response:
            response.raise_for_status()
            return await response.json(content_type=None)

    async def get_metric(self, metric_name):
        metrics = await self.get_metrics(metric_name)
        assert metric_name in metrics, (
            f'No metric with name {metric_name!r}. '
            f'Use "single_metric" function instead of "get_metric"'
        )
        return metrics[metric_name]

    async def metrics_raw(
            self,
            output_format,
            *,
            path: str = None,
            prefix: str = None,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> str:
        if not self._config.server_monitor_path:
            raise ConfigurationError(
                'handler-server-monitor component is not configured',
            )

        params = {'format': output_format}
        if prefix:
            params['prefix'] = prefix

        if path:
            params['path'] = path

        if labels:
            params['labels'] = json.dumps(labels)

        response = await self.get(
            self._config.server_monitor_path, params=params,
        )
        async with response:
            response.raise_for_status()
            return await response.text()

    async def metrics(
            self,
            *,
            path: str = None,
            prefix: str = None,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> metric_module.MetricsSnapshot:
        response = await self.metrics_raw(
            output_format='json', path=path, prefix=prefix, labels=labels,
        )
        return metric_module.MetricsSnapshot.from_json(str(response))

    async def single_metric_optional(
            self,
            path: str,
            *,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> typing.Optional[Metric]:
        response = await self.metrics(path=path, labels=labels)
        metrics_list = response.get(path, [])

        assert len(metrics_list) <= 1, (
            f'More than one metric found for path {path} and labels {labels}: '
            f'{response}',
        )

        if not metrics_list:
            return None

        return next(iter(metrics_list))

    async def single_metric(
            self,
            path: str,
            *,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> Metric:
        value = await self.single_metric_optional(path, labels=labels)
        assert value is not None, (
            f'No metric was found for path {path} and labels {labels}',
        )
        return value


# @endcond


class ClientMonitor(ClientWrapper):
    """
    Asyncio userver client for monitor listeners, typically retrieved from
    plugins.service_client.monitor_client fixture.

    Compatible with werkzeug interface.

    @ingroup userver_testsuite
    """

    @_wrap_client_error
    async def metrics(
            self,
            *,
            path: str = None,
            prefix: str = None,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> metric_module.MetricsSnapshot:
        """
        Returns a dict of metric names to Metric.

        @param path Optional full metric path
        @param prefix Optional prefix on which the metric paths should start
        @param labels Optional dictionary of labels that must be in the metric
        """
        return await self._client.metrics(
            path=path, prefix=prefix, labels=labels,
        )

    @_wrap_client_error
    async def single_metric_optional(
            self,
            path: str,
            *,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> typing.Optional[Metric]:
        """
        Either return a Metric or None if there's no such metric.

        @param path Full metric path
        @param labels Optional dictionary of labels that must be in the metric

        @throws AssertionError if more than one metric returned
        """
        return await self._client.single_metric_optional(path, labels=labels)

    @_wrap_client_error
    async def single_metric(
            self,
            path: str,
            *,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> typing.Optional[Metric]:
        """
        Returns the Metric.

        @param path Full metric path
        @param labels Optional dictionary of labels that must be in the metric

        @throws AssertionError if more than one metric or no metric found
        """
        return await self._client.single_metric(path, labels=labels)

    @_wrap_client_error
    async def metrics_raw(
            self,
            output_format: str,
            *,
            path: str = None,
            prefix: str = None,
            labels: typing.Optional[typing.Dict[str, str]] = None,
    ) -> typing.Dict[str, Metric]:
        """
        Low level function that returns metrics in a specific format.
        Use `metrics` and `single_metric` instead if possible.

        @param output_format Metric output format. See
               server::handlers::ServerMonitor for a list of supported formats.
        @param path Optional full metric path
        @param prefix Optional prefix on which the metric paths should start
        @param labels Optional dictionary of labels that must be in the metric
        """
        return await self._client.metrics_raw(
            output_format=output_format,
            path=path,
            prefix=prefix,
            labels=labels,
        )

    @_wrap_client_error
    async def get_metrics(self, prefix=None):
        """
        @deprecated Use metrics() or single_metric() instead
        """
        return await self._client.get_metrics(prefix=prefix)

    @_wrap_client_error
    async def get_metric(self, metric_name):
        """
        @deprecated Use metrics() or single_metric() instead
        """
        return await self._client.get_metric(metric_name)


# @cond


class AiohttpClient(service_client.AiohttpClient):
    PeriodicTaskFailed = PeriodicTaskFailed
    TestsuiteActionFailed = TestsuiteActionFailed
    TestsuiteTaskNotFound = TestsuiteTaskNotFound
    TestsuiteTaskConflict = TestsuiteTaskConflict
    TestsuiteTaskFailed = TestsuiteTaskFailed

    def __init__(
            self,
            base_url: str,
            *,
            config: TestsuiteClientConfig,
            mocked_time,
            log_capture_fixture,
            testpoint,
            testpoint_control,
            cache_invalidation_state,
            span_id_header=None,
            api_coverage_report=None,
            periodic_tasks_state: typing.Optional[PeriodicTasksState] = None,
            **kwargs,
    ):
        super().__init__(base_url, span_id_header=span_id_header, **kwargs)
        self._config = config
        self._periodic_tasks = periodic_tasks_state
        self._testpoint = testpoint
        self._log_capture_fixture = log_capture_fixture
        self._state_manager = _StateManager(
            mocked_time=mocked_time,
            testpoint=self._testpoint,
            testpoint_control=testpoint_control,
            invalidation_state=cache_invalidation_state,
        )
        self._api_coverage_report = api_coverage_report

    async def run_periodic_task(self, name):
        response = await self._testsuite_action('run_periodic_task', name=name)
        if not response['status']:
            raise self.PeriodicTaskFailed(f'Periodic task {name} failed')

    async def suspend_periodic_tasks(self, names: typing.List[str]) -> None:
        if not self._periodic_tasks:
            raise ConfigurationError('No periodic_tasks_state given')
        self._periodic_tasks.tasks_to_suspend.update(names)
        await self._suspend_periodic_tasks()

    async def resume_periodic_tasks(self, names: typing.List[str]) -> None:
        if not self._periodic_tasks:
            raise ConfigurationError('No periodic_tasks_state given')
        self._periodic_tasks.tasks_to_suspend.difference_update(names)
        await self._suspend_periodic_tasks()

    async def resume_all_periodic_tasks(self) -> None:
        if not self._periodic_tasks:
            raise ConfigurationError('No periodic_tasks_state given')
        self._periodic_tasks.tasks_to_suspend.clear()
        await self._suspend_periodic_tasks()

    async def write_cache_dumps(self, names: typing.List[str]) -> None:
        await self._testsuite_action('write_cache_dumps', names=names)

    async def read_cache_dumps(self, names: typing.List[str]) -> None:
        await self._testsuite_action('read_cache_dumps', names=names)

    async def run_distlock_task(self, name: str) -> None:
        await self.run_task(f'distlock/{name}')

    async def reset_metrics(self) -> None:
        await self._testsuite_action('reset_metrics')

    async def metrics_portability(
            self, *, prefix: typing.Optional[str] = None,
    ) -> typing.Dict[str, typing.List[typing.Dict[str, str]]]:
        return await self._testsuite_action(
            'metrics_portability', prefix=prefix,
        )

    async def list_tasks(self) -> typing.List[str]:
        response = await self._do_testsuite_action('tasks_list')
        async with response:
            response.raise_for_status()
            body = await response.json(content_type=None)
            return body['tasks']

    async def run_task(self, name: str) -> None:
        response = await self._do_testsuite_action(
            'task_run', json={'name': name},
        )
        await _task_check_response(name, response)

    @contextlib.asynccontextmanager
    async def spawn_task(self, name: str):
        task_id = await self._task_spawn(name)
        try:
            yield
        finally:
            await self._task_stop_spawned(task_id)

    async def _task_spawn(self, name: str) -> str:
        response = await self._do_testsuite_action(
            'task_spawn', json={'name': name},
        )
        data = await _task_check_response(name, response)
        return data['task_id']

    async def _task_stop_spawned(self, task_id: str) -> None:
        response = await self._do_testsuite_action(
            'task_stop', json={'task_id': task_id},
        )
        await _task_check_response(task_id, response)

    @contextlib.asynccontextmanager
    async def capture_logs(self):
        async with self._log_capture_fixture.start_capture() as capture:
            await self._testsuite_action(
                'log_capture', socket_logging_duplication=True,
            )
            try:
                yield capture
            finally:
                await self._testsuite_action(
                    'log_capture', socket_logging_duplication=False,
                )

    async def invalidate_caches(
            self,
            *,
            clean_update: bool = True,
            cache_names: typing.Optional[typing.List[str]] = None,
    ) -> None:
        await self.tests_control(
            invalidate_caches=True,
            clean_update=clean_update,
            cache_names=cache_names,
        )

    async def tests_control(
            self,
            *,
            invalidate_caches: bool = True,
            clean_update: bool = True,
            cache_names: typing.Optional[typing.List[str]] = None,
            http_allowed_urls_extra=None,
    ) -> typing.Dict[str, typing.Any]:
        body: typing.Dict[
            str, typing.Any,
        ] = self._state_manager.get_pending_update()

        if 'invalidate_caches' in body and invalidate_caches:
            if not clean_update or cache_names:
                logger.warning(
                    'Manual cache invalidation leads to indirect initial '
                    'full cache invalidation',
                )
                await self._prepare()
                body = {}

        if invalidate_caches:
            body['invalidate_caches'] = {
                'update_type': ('full' if clean_update else 'incremental'),
            }
            if cache_names:
                body['invalidate_caches']['names'] = cache_names
        if http_allowed_urls_extra is not None:
            body['http_allowed_urls_extra'] = http_allowed_urls_extra
        return await self._tests_control(body)

    async def update_server_state(self) -> None:
        await self._prepare()

    async def enable_testpoints(self, *, no_auto_cache_cleanup=False) -> None:
        if not self._testpoint:
            return
        if no_auto_cache_cleanup:
            await self._tests_control(
                {'testpoints': sorted(self._testpoint.keys())},
            )
        else:
            await self.update_server_state()

    async def _tests_control(self, body: dict) -> typing.Dict[str, typing.Any]:
        with self._state_manager.updating_state(body):
            async with await self._do_testsuite_action(
                    'control', json=body, testsuite_skip_prepare=True,
            ) as response:
                if response.status == 404:
                    raise ConfigurationError(
                        'It seems that testsuite support is not enabled '
                        'for your service',
                    )
                response.raise_for_status()
                return await response.json(content_type=None)

    async def _suspend_periodic_tasks(self):
        if (
                self._periodic_tasks.tasks_to_suspend
                != self._periodic_tasks.suspended_tasks
        ):
            await self._testsuite_action(
                'suspend_periodic_tasks',
                names=sorted(self._periodic_tasks.tasks_to_suspend),
            )
            self._periodic_tasks.suspended_tasks = set(
                self._periodic_tasks.tasks_to_suspend,
            )

    def _do_testsuite_action(self, action, **kwargs):
        if not self._config.testsuite_action_path:
            raise ConfigurationError(
                'tests-control component is not properly configured',
            )
        path = self._config.testsuite_action_path.format(action=action)
        return self.post(path, **kwargs)

    async def _testsuite_action(
            self, action, *, testsuite_skip_prepare=False, **kwargs,
    ):
        async with await self._do_testsuite_action(
                action,
                json=kwargs,
                testsuite_skip_prepare=testsuite_skip_prepare,
        ) as response:
            if response.status == 500:
                raise TestsuiteActionFailed
            response.raise_for_status()
            return await response.json(content_type=None)

    async def _prepare(self) -> None:
        pending_update = self._state_manager.get_pending_update()
        if pending_update:
            await self._tests_control(pending_update)

    async def _request(  # pylint: disable=arguments-differ
            self,
            http_method: str,
            path: str,
            headers: typing.Optional[typing.Dict[str, str]] = None,
            bearer: typing.Optional[str] = None,
            x_real_ip: typing.Optional[str] = None,
            *,
            testsuite_skip_prepare: bool = False,
            **kwargs,
    ) -> aiohttp.ClientResponse:
        if not testsuite_skip_prepare:
            await self._prepare()

        response = await super()._request(
            http_method, path, headers, bearer, x_real_ip, **kwargs,
        )
        if self._api_coverage_report:
            self._api_coverage_report.update_usage_stat(
                path, http_method, response.status, response.content_type,
            )

        return response


# @endcond


class Client(ClientWrapper):
    """
    Asyncio userver client, typically retrieved from
    @ref service_client "plugins.service_client.service_client"
    fixture.

    Compatible with werkzeug interface.

    @ingroup userver_testsuite
    """

    PeriodicTaskFailed = PeriodicTaskFailed
    TestsuiteActionFailed = TestsuiteActionFailed
    TestsuiteTaskNotFound = TestsuiteTaskNotFound
    TestsuiteTaskConflict = TestsuiteTaskConflict
    TestsuiteTaskFailed = TestsuiteTaskFailed

    def _wrap_client_response(
            self, response: aiohttp.ClientResponse,
    ) -> typing.Awaitable[http.ClientResponse]:
        return http.wrap_client_response(
            response, json_loads=approx.json_loads,
        )

    @_wrap_client_error
    async def run_periodic_task(self, name):
        await self._client.run_periodic_task(name)

    @_wrap_client_error
    async def suspend_periodic_tasks(self, names: typing.List[str]) -> None:
        await self._client.suspend_periodic_tasks(names)

    @_wrap_client_error
    async def resume_periodic_tasks(self, names: typing.List[str]) -> None:
        await self._client.resume_periodic_tasks(names)

    @_wrap_client_error
    async def resume_all_periodic_tasks(self) -> None:
        await self._client.resume_all_periodic_tasks()

    @_wrap_client_error
    async def write_cache_dumps(self, names: typing.List[str]) -> None:
        await self._client.write_cache_dumps(names=names)

    @_wrap_client_error
    async def read_cache_dumps(self, names: typing.List[str]) -> None:
        await self._client.read_cache_dumps(names=names)

    async def run_task(self, name: str) -> None:
        await self._client.run_task(name)

    async def run_distlock_task(self, name: str) -> None:
        await self._client.run_distlock_task(name)

    async def reset_metrics(self) -> None:
        """
        Calls `ResetMetric(metric);` for each metric that has such C++ function
        """
        await self._client.reset_metrics()

    async def metrics_portability(
            self, *, prefix: typing.Optional[str] = None,
    ) -> typing.Dict[str, typing.List[typing.Dict[str, str]]]:
        """
        Reports metrics related issues that could be encountered on
        different monitoring systems.

        @sa @ref utils::statistics::GetPortabilityInfo
        """
        return await self._client.metrics_portability(prefix=prefix)

    def list_tasks(self) -> typing.List[str]:
        return self._client.list_tasks()

    def spawn_task(self, name: str):
        return self._client.spawn_task(name)

    def capture_logs(self):
        return self._client.capture_logs()

    @_wrap_client_error
    async def invalidate_caches(
            self,
            *,
            clean_update: bool = True,
            cache_names: typing.Optional[typing.List[str]] = None,
    ) -> None:
        """
        Send request to service to update caches.

        @param clean_update if False, service will do a faster incremental
               update of caches whenever possible.
        @param cache_names which caches specifically should be updated;
               update all if None.
        """
        await self._client.invalidate_caches(
            clean_update=clean_update, cache_names=cache_names,
        )

    @_wrap_client_error
    async def tests_control(
            self, *args, **kwargs,
    ) -> typing.Dict[str, typing.Any]:
        return await self._client.tests_control(*args, **kwargs)

    @_wrap_client_error
    async def update_server_state(self) -> None:
        """
        Update service-side state through http call to 'tests/control':
        - clear dirty (from other tests) caches
        - set service-side mocked time,
        - resume / suspend periodic tasks
        - enable testpoints
        If service is up-to-date, does nothing.
        """
        await self._client.update_server_state()

    @_wrap_client_error
    async def enable_testpoints(self, *args, **kwargs) -> None:
        """
        Send list of handled testpoint pats to service. For these paths service
        will no more skip http calls from TESTPOINT(...) macro.

        @param no_auto_cache_cleanup prevent automatic cache cleanup.
        When calling service client first time in scope of current test, client
        makes additional http call to `tests/control` to update caches, to get
        rid of data from previous test.
        """
        await self._client.enable_testpoints(*args, **kwargs)


@dataclasses.dataclass
class _State:
    """Reflects the (supposed) current service state."""

    invalidation_state: caches.InvalidationState
    now: typing.Optional[str] = _UNKNOWN_STATE
    testpoints: typing.FrozenSet[str] = frozenset([_UNKNOWN_STATE])


class _StateManager:
    """
    Used for computing the requests that we need to automatically align
    the service state with the test fixtures state.
    """

    def __init__(
            self,
            *,
            mocked_time,
            testpoint,
            testpoint_control,
            invalidation_state: caches.InvalidationState,
    ):
        self._state = _State(
            invalidation_state=copy.deepcopy(invalidation_state),
        )
        self._mocked_time = mocked_time
        self._testpoint = testpoint
        self._testpoint_control = testpoint_control
        self._invalidation_state = invalidation_state

    @contextlib.contextmanager
    def updating_state(self, body: typing.Dict[str, typing.Any]):
        """
        Whenever `tests_control` handler is invoked
        (by the client itself during `prepare` or manually by the user),
        we need to synchronize `_state` with the (supposed) service state.
        The state update is decoded from the request body.
        """
        saved_state = copy.deepcopy(self._state)
        try:
            self._update_state(body)
            self._apply_new_state()
            yield
        except:  # noqa
            self._state = saved_state
            self._apply_new_state()
            raise

    def get_pending_update(self) -> typing.Dict[str, typing.Any]:
        """
        Compose the body of the `tests_control` request required to completely
        synchronize the service state with the state of test fixtures.
        """
        body: typing.Dict[str, typing.Any] = {}

        if self._invalidation_state.has_caches_to_update:
            body['invalidate_caches'] = {'update_type': 'full'}
            if not self._invalidation_state.should_update_all_caches:
                body['invalidate_caches']['names'] = list(
                    self._invalidation_state.caches_to_update,
                )

        desired_testpoints = self._testpoint.keys()
        if self._state.testpoints != frozenset(desired_testpoints):
            body['testpoints'] = sorted(desired_testpoints)

        desired_now = self._get_desired_now()
        if self._state.now != desired_now:
            body['mock_now'] = desired_now

        return body

    def _update_state(self, body: typing.Dict[str, typing.Any]) -> None:
        body_invalidate_caches = body.get('invalidate_caches', {})
        update_type = body_invalidate_caches.get('update_type', 'full')
        body_cache_names = body_invalidate_caches.get('names', None)
        # An incremental update is considered insufficient to bring a cache
        # to a known state.
        if body_invalidate_caches and update_type == 'full':
            if body_cache_names is None:
                self._state.invalidation_state.on_all_caches_updated()
            else:
                self._state.invalidation_state.on_caches_updated(
                    body_cache_names,
                )

        if 'mock_now' in body:
            self._state.now = body['mock_now']

        testpoints: typing.Optional[typing.List[str]] = body.get(
            'testpoints', None,
        )
        if testpoints is not None:
            self._state.testpoints = frozenset(testpoints)

    def _apply_new_state(self):
        """Apply new state to related components."""
        self._testpoint_control.enabled_testpoints = self._state.testpoints
        self._invalidation_state.assign_copy(self._state.invalidation_state)

    def _get_desired_now(self) -> typing.Optional[str]:
        if self._mocked_time.is_enabled:
            return utils.timestring(self._mocked_time.now())
        return None


async def _task_check_response(name: str, response) -> dict:
    async with response:
        if response.status == 404:
            raise TestsuiteTaskNotFound(f'Testsuite task {name!r} not found')
        if response.status == 409:
            raise TestsuiteTaskConflict(f'Testsuite task {name!r} conflict')
        assert response.status == 200
        data = await response.json()
        if not data.get('status', True):
            raise TestsuiteTaskFailed(name, data['reason'])
        return data
