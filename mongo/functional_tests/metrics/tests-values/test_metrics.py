import contextlib
import dataclasses
import typing

import pytest
from pytest_userver import metrics  # pylint: disable=import-error


# Note: does not work for percentile metrics
@dataclasses.dataclass
class MetricsDiff:
    before: metrics.MetricsSnapshot
    after: typing.Optional[metrics.MetricsSnapshot]

    def value_at(self, **kwargs):
        return self.after.value_at(**kwargs) - self.before.value_at(**kwargs)


@pytest.fixture
def collect_metrics(service_client, monitor_client, mocked_time):
    @contextlib.asynccontextmanager
    async def func(**kwargs):
        # Let old RecentPeriod metrics fall out of range
        mocked_time.sleep(66)
        await service_client.update_server_state()

        before = await monitor_client.metrics(**kwargs)
        diff = MetricsDiff(before=before, after=None)
        yield diff

        # Let the new RecentPeriod metrics register
        mocked_time.sleep(6)
        await service_client.update_server_state()

        diff.after = await monitor_client.metrics(**kwargs)

    return func


async def test_metrics_values(service_client, collect_metrics):
    async with collect_metrics(prefix='mongo') as snapshot:
        # In case the previous tests put 'key=foo'
        response = await service_client.delete('/v1/key-value?key=foo')
        assert response.status == 200

        response = await service_client.get('/v1/key-value?key=foo')
        assert response.status == 200
        assert response.text == '0'

        response = await service_client.put('/v1/key-value?key=foo&value=bar')
        assert response.status == 200

        response = await service_client.get('/v1/key-value?key=foo')
        assert response.status == 200
        assert response.text == '1'

        # Intentionally causing a duplicate key error
        response = await service_client.put('/v1/key-value?key=foo&value=bar')
        assert response.status == 500

    assert (
        snapshot.value_at(
            path='mongo.by-operation.errors',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'write',
                'mongo_operation': 'insert-one',
                'mongo_error': 'duplicate-key',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-operation.errors-total',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'write',
                'mongo_operation': 'insert-one',
                'mongo_error': 'total',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-collection.errors',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'write',
                'mongo_error': 'duplicate-key',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-collection.errors-total',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'write',
                'mongo_error': 'total',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-database.errors',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_direction': 'write',
                'mongo_error': 'duplicate-key',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-database.errors-total',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_direction': 'write',
                'mongo_error': 'total',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-operation.success',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'write',
                'mongo_operation': 'insert-one',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-operation.success',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'write',
                'mongo_operation': 'delete-one',
            },
            default=0,
        )
        == 1
    )

    assert (
        snapshot.value_at(
            path='mongo.by-operation.success',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'read',
                'mongo_operation': 'count',
            },
            default=0,
        )
        == 2
    )

    assert (
        snapshot.value_at(
            path='mongo.by-collection.success',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'write',
            },
            default=0,
        )
        == 2
    )

    assert (
        snapshot.value_at(
            path='mongo.by-collection.success',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_collection': 'test',
                'mongo_direction': 'read',
            },
            default=0,
        )
        == 2
    )

    assert (
        snapshot.value_at(
            path='mongo.by-database.success',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_direction': 'write',
            },
            default=0,
        )
        == 2
    )

    assert (
        snapshot.value_at(
            path='mongo.by-database.success',
            labels={
                'mongo_database': 'key-value-database',
                'mongo_direction': 'read',
            },
            default=0,
        )
        == 2
    )
