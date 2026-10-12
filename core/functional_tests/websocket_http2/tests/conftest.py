import contextlib
import socket

import h2.config
import h2.connection
import h2.events
import pytest
import wsproto.connection
import wsproto.events

pytest_plugins = ['pytest_userver.plugins.core']


class Rfc8441Error(Exception):
    pass


class Rfc8441Client:
    """A websocket bootstrapped over HTTP/2.0 with the extended CONNECT of RFC 8441.

    Hand-rolled because no mainstream Python websocket client speaks RFC 8441: the
    handshake is done with `h2`, and `wsproto` provides plain RFC 6455 framing for
    the bytes that then flow inside the DATA frames of the stream.
    """

    # Generous: a sanitized service on a loaded machine is slow, and a real hang still
    # fails the test, only later.
    def __init__(self, host: str, port: int, timeout: float = 60.0):
        self._authority = f'{host}:{port}'
        self._sock = socket.create_connection((host, port), timeout=timeout)
        self._conn = h2.connection.H2Connection(
            config=h2.config.H2Configuration(client_side=True),
        )
        self._conn.initiate_connection()
        self._flush()
        self._ws = wsproto.connection.Connection(
            wsproto.connection.ConnectionType.CLIENT,
        )
        self._stream_id = None
        self._pending = []
        self._got_settings = False

    # --- HTTP/2.0 plumbing ---

    def _flush(self):
        self._sock.sendall(self._conn.data_to_send())

    def _pump(self):
        """Reads one batch of server bytes and returns the resulting h2 events."""
        data = self._sock.recv(65535)
        if not data:
            raise Rfc8441Error('the server closed the connection')
        events = self._conn.receive_data(data)
        self._flush()
        return events

    @property
    def enable_connect_protocol(self) -> int:
        # The local default is 0, so wait for the server SETTINGS to actually arrive
        # instead of reporting "not advertised" before it had a chance to.
        while not self._got_settings:
            for event in self._pump():
                if isinstance(event, h2.events.RemoteSettingsChanged):
                    self._got_settings = True
                self._remember(event)
        return self._conn.remote_settings.enable_connect_protocol

    def request(self, path: str, method: str = 'GET') -> int:
        """Starts an ordinary request on its own stream and returns its id."""
        stream_id = self._conn.get_next_available_stream_id()
        self._conn.send_headers(
            stream_id,
            [
                (':method', method),
                (':scheme', 'http'),
                (':path', path),
                (':authority', self._authority),
            ],
            end_stream=True,
        )
        self._flush()
        return stream_id

    def status_of(self, stream_id: int) -> str:
        status = None
        while status is None:
            # The whole batch has to be consumed even once the status is known: the
            # server may well have put the response headers and the first bytes of the
            # tunnelled protocol into one TCP segment.
            for event in self._pump():
                if isinstance(event, h2.events.ResponseReceived) and event.stream_id == stream_id:
                    status = dict(event.headers)[b':status'].decode()
                else:
                    self._remember(event)
        return status

    # --- RFC 8441 ---

    def connect(self, path: str, extra_headers=()) -> str:
        """Sends the extended CONNECT and returns the response `:status`."""
        assert self._stream_id is None, 'the client drives a single websocket'
        self._stream_id = self._conn.get_next_available_stream_id()
        self._conn.send_headers(
            self._stream_id,
            [
                (':method', 'CONNECT'),
                (':protocol', 'websocket'),
                (':scheme', 'http'),
                (':path', path),
                (':authority', self._authority),
                ('sec-websocket-version', '13'),
                *extra_headers,
            ],
            # The stream stays open for the lifetime of the websocket.
            end_stream=False,
        )
        self._flush()

        return self.status_of(self._stream_id)

    def _remember(self, event):
        if isinstance(event, h2.events.DataReceived) and event.stream_id == self._stream_id:
            if event.flow_controlled_length:
                self._conn.acknowledge_received_data(
                    event.flow_controlled_length,
                    event.stream_id,
                )
                self._flush()
            if event.data:
                self._ws.receive_data(event.data)
                self._pending.extend(self._ws.events())
        elif isinstance(event, h2.events.StreamEnded) and event.stream_id == self._stream_id:
            # Half-closing the stream is how the transport under the websocket goes away.
            self._ws.receive_data(None)
            self._pending.extend(self._ws.events())
        elif isinstance(event, h2.events.StreamReset) and event.stream_id == self._stream_id:
            raise Rfc8441Error(f'the websocket stream was reset: {event.error_code}')
        elif isinstance(event, h2.events.ConnectionTerminated):
            raise Rfc8441Error(f'the connection was terminated: {event.error_code}')

    def send(self, event):
        self._conn.send_data(self._stream_id, self._ws.send(event))
        self._flush()

    def send_text(self, payload: str):
        self.send(wsproto.events.TextMessage(data=payload))

    def send_bytes(self, payload: bytes):
        self.send(wsproto.events.BytesMessage(data=payload))

    def recv(self):
        """Returns one websocket event, reassembling fragmented messages."""
        parts = None
        while True:
            while self._pending:
                event = self._pending.pop(0)
                if not isinstance(event, wsproto.events.Message):
                    return event
                parts = event.data if parts is None else parts + event.data
                if event.message_finished:
                    return type(event)(data=parts)
            for event in self._pump():
                self._remember(event)

    def recv_message(self):
        event = self.recv()
        assert isinstance(event, wsproto.events.Message), event
        return event.data

    def close(self, code: int = 1000):
        self.send(wsproto.events.CloseConnection(code=code))
        return self.recv()

    def disconnect(self):
        self._sock.close()


@pytest.fixture(name='rfc8441_client')
async def _rfc8441_client(service_client, service_port):
    # `service_client` is required so that the daemon is up before we connect: the
    # client speaks to the listener directly, bypassing the testsuite plumbing.
    clients = []

    @contextlib.contextmanager
    def make_client():
        client = Rfc8441Client('localhost', service_port)
        clients.append(client)
        try:
            yield client
        finally:
            client.disconnect()

    yield make_client

    for client in clients:
        client.disconnect()
