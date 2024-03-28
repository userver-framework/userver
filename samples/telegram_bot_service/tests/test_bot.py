def get_private_message(user_id, message_id, date, text,
                        first_name="first_name", last_name="last_name",
                        username="username"):
    return {
        "message_id": message_id,
        "from": {
            "id":user_id,
            "is_bot":False,
            "first_name": first_name,
            "last_name": last_name,
            "username": username,
        },
        "chat": {
            "id": user_id,
            "first_name": first_name,
            "last_name": last_name,
            "username": username,
            "type":"private"
        },
        "date": date,
        "text": text
    }

async def test_greeting(service_client, mockserver, updates, tg_method):
    user_id = 1
    first_name = '(0,0)'
    start_message_id = 144
    date = 1696763977

    updates.append({
        "update_id": 316773233,
        "message": get_private_message(user_id, start_message_id, date,
                                      "hello", first_name=first_name)
    })

    @mockserver.json_handler(tg_method('sendChatAction'))
    def send_chat_action_mock(*, body_json):
        assert body_json['chat_id'] == user_id
        assert body_json['action'] == 'upload_photo'

        return {
          'ok': True,
          'result': True
        }

    @mockserver.aiohttp_json_handler(tg_method('sendPhoto'))
    async def send_photo_mock(request):
        assert request.content_type == 'multipart/form-data'
        multipart_reader = await request.multipart()

        text_content_types = [
            'application/json; charset=utf-8',
            'text/plain; charset=utf-8'
        ]

        part = await multipart_reader.next()
        assert part.name == 'chat_id'
        assert 'content-type' in part.headers
        assert part.headers['content-type'] in text_content_types
        assert await part.text() == '1'

        part = await multipart_reader.next()
        assert part.name == 'photo'
        assert 'content-type' in part.headers
        assert part.headers['content-type'] == 'image/jpeg'
        assert len(await part.read()) == 67028

        part = await multipart_reader.next()
        assert part.name == 'caption'
        assert 'content-type' in part.headers
        assert part.headers['content-type'] in text_content_types
        greeting = 'Hello, {}! '.format(first_name) + \
                   'This is a test telegram bot written on userver.'
        assert await part.text() == greeting

        part = await multipart_reader.next()
        assert part.name == 'caption_entities'
        assert 'content-type' in part.headers
        assert part.headers['content-type'] == 'application/json; charset=utf-8'
        assert await part.json() == [{
            'length': 7,
            'offset': 48 + len(first_name),
            'type': 'text_link',
            'url': 'https://userver.tech/index.html'
        }]

        assert await multipart_reader.next() is None

        return {
            'ok': True,
            'result': {
                'message_id': start_message_id + 1,
                'date': date + 3,
                'from': {
                    'id': 123456,
                    'is_bot': True,
                    'first_name': 'TestBot'
                }
            }
        }

    await mockserver.get_callqueue_for(tg_method('sendPhoto')).wait_call(2)
