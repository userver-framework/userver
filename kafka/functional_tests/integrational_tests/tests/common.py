from typing import Dict
from typing import List


def generate_messages_to_consume(
    topics: List[str],
    cnt: int,
) -> Dict[str, List[Dict[str, str]]]:
    messages: Dict[str, List[Dict[str, str]]] = {}
    for topic in topics:
        messages[topic] = [
            {
                "topic": topic,
                "key": f"test-key-{i}",
                "payload": f"test-value-{i}",
            }
            for i in range(cnt)
        ]

    return messages
