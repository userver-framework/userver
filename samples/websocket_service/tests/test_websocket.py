# /// [Functional test]
async def test_echo(websocket_client):
    async with websocket_client.get('chat') as chat:
        await chat.send('hello')
        response = await chat.recv()
        assert response == 'hello'
        # /// [Functional test]
