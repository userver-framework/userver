import base64

import pytest


@pytest.mark.skip(reason='restore in TAXICOMMON-8860')
async def test_topic(service_client, ydb, testpoint):
    @testpoint('topic-handle-message')
    async def handle_message_testpoint(data):
        pass

    await service_client.enable_testpoints()

    ydb.execute(
        """
        UPSERT INTO `records` (id, name, str, num)
        VALUES ("test-topic-id", "test-topic-name", "test-topic-str", 321);
    """,
    )

    upsert_record_args = await handle_message_testpoint.wait_call()

    upsert_record_message = upsert_record_args['data']
    assert upsert_record_message['key'] == ['test-topic-id', 'test-topic-name']
    assert (
        base64.b64decode(upsert_record_message['newImage']['str'])
        == b'test-topic-str'
    )
    assert upsert_record_message['newImage']['num'] == 321

    ydb.execute(
        """
        DELETE FROM `records`
        WHERE id = "test-topic-id" AND name = "test-topic-name";
    """,
    )

    delete_record_args = await handle_message_testpoint.wait_call()

    delete_record_message = delete_record_args['data']
    assert delete_record_message['key'] == ['test-topic-id', 'test-topic-name']
    assert 'newImage' not in delete_record_message
