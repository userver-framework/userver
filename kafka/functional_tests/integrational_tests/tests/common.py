def generate_messages_to_consume(
        topics: list[str], cnt: int,
) -> dict[str, list[dict[str, str]]]:
    messages: dict[str, list[dict[str, str]]] = {}
    for topic in topics:
        messages[topic] = [
            {
                'topic': topic,
                'key': f'test-key-{i}',
                'payload': f'test-value-{i}',
            }
            for i in range(cnt)
        ]

    return messages
