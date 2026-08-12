# /// [YDB topic writer service sample - testsuite conftest]
import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins.ydb']


@pytest.fixture(scope='session')
def ydb_topic_path() -> str:
    return 'messages'


@pytest.fixture(scope='session')
def ydb_topic_consumer_name() -> str:
    return 'test-consumer'


@pytest.fixture(autouse=True, scope='function')
def ydb_create_topic(ydb, ydb_topic_path, ydb_topic_consumer_name):
    topic_client = ydb.topic_client

    try:
        describe_result = topic_client.describe_topic(ydb_topic_path)
        if hasattr(describe_result, 'result'):
            describe_result = describe_result.result()
        _ = describe_result
    except Exception:
        create_result = topic_client.create_topic(
            ydb_topic_path,
            consumers=[ydb_topic_consumer_name],
        )
        if hasattr(create_result, 'result'):
            create_result.result()
            # /// [YDB topic writer service sample - testsuite conftest]
