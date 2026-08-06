import asyncio


# /// [YDB topic writer service sample - functional test]
async def test_topic_writer_cycle(service_client, ydb, ydb_topic_path, ydb_topic_consumer_name):
    message = 'hello topic writer'

    response = await service_client.post('/write', data=message)
    assert response.status == 200
    assert response.text == 'Message was written to topic\n'

    topic_client = ydb.topic_client
    reader = topic_client.reader(
        ydb_topic_path,
        consumer=ydb_topic_consumer_name,
    )

    deadline = asyncio.get_running_loop().time() + 10.0
    received_messages = []
    while asyncio.get_running_loop().time() < deadline:
        message_batch = reader.receive_message(timeout=1.0)
        if hasattr(message_batch, 'result'):
            message_batch = message_batch.result()

        if not message_batch:
            await asyncio.sleep(0.2)
            continue

        if isinstance(message_batch, (list, tuple)):
            received_messages.extend(message_batch)
        else:
            received_messages.append(message_batch)

        for received in received_messages:
            data = getattr(received, 'data', None)
            if callable(data):
                data = data()
            if data == message.encode():
                if hasattr(reader, 'commit'):
                    commit_result = reader.commit(received)
                    if hasattr(commit_result, 'result'):
                        commit_result.result()
                if hasattr(reader, 'close'):
                    close_result = reader.close()
                    if hasattr(close_result, 'result'):
                        close_result.result()
                return

    if hasattr(reader, 'close'):
        close_result = reader.close()
        if hasattr(close_result, 'result'):
            close_result.result()

    rendered = []
    for received in received_messages:
        data = getattr(received, 'data', None)
        if callable(data):
            data = data()
        rendered.append(data)
    raise AssertionError(f'Message `{message}` was not found in topic messages: {rendered}')
    # /// [YDB topic writer service sample - functional test]
