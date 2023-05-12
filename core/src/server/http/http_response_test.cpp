#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <server/http/http2_callbacks.hpp>
#include <server/http/http_request_impl.hpp>
#include <server/http/http_response_writer.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/engine/async.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(HttpResponse, Smoke) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  server::request::ResponseDataAccounter accounter;
  server::http::HttpRequestImpl request{accounter};
  server::http::HttpResponse response{request, accounter};

  constexpr std::string_view kBody = "test data";
  response.SetData(std::string{kBody});
  response.SetStatus(server::http::HttpStatus::kOk);

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);
  auto send_task = engine::AsyncNoSpan(
      [](auto&& response, auto&& socket) { response.SendResponse(socket); },
      std::ref(response), std::move(server));

  std::vector<char> buffer(4096, '\0');
  const auto reply_size =
      client.RecvAll(buffer.data(), buffer.size(), test_deadline);

  std::string_view reply{buffer.data(), reply_size};
  constexpr std::string_view expected_header = "HTTP/1.1 200 OK\r\n";
  ASSERT_EQ(reply.substr(0, expected_header.size()), expected_header);
  const auto expected_content_length = fmt::format(
      "\r\n{}: {}\r\n", http::headers::kContentLength, kBody.size());
  EXPECT_TRUE(reply.find(expected_content_length) != std::string_view::npos);

  EXPECT_EQ(reply.substr(reply.size() - 4 - kBody.size()),
            fmt::format("\r\n\r\n{}", kBody));
}
// DO NOT DELETE NGHTTP2_NV_FLAG_NO_INDEX
// https://nghttp2.org/documentation/tutorial-hpack.html

struct ReceivedDataStorage {
  std::vector<std::pair<std::string, std::string>> headers{};
  std::string data{};
  bool stream_closed{false};
};

// implements nghttp2_on_header_callback
int StoreReceivedHeader(nghttp2_session*, const nghttp2_frame*,
                        const uint8_t* name, size_t, const uint8_t* value,
                        size_t, uint8_t, void* user_data) {
  auto& result = *static_cast<ReceivedDataStorage*>(user_data);
  result.headers.emplace_back(reinterpret_cast<const char*>(name),
                              reinterpret_cast<const char*>(value));
  // const auto [_, insertion_happened] = result.headers.emplace(
  //         reinterpret_cast<const char*>(name),
  //         reinterpret_cast<const char*>(value));
  // EXPECT_TRUE(insertion_happened) << "header name must be unique";
  return 0;
}

// implements nghttp2_on_data_chunk_recv_callback
int StoreReceivedData(nghttp2_session*, uint8_t, int32_t, const uint8_t* data,
                      size_t len, void* user_data) {
  auto& result = *static_cast<ReceivedDataStorage*>(user_data);
  result.data += std::string_view{reinterpret_cast<const char*>(data), len};
  return 0;
}

// implements nghttp2_on_data_chunk_recv_callback
int HandleStreamClosed(nghttp2_session*, int32_t, uint32_t, void* user_data) {
  auto& result = *static_cast<ReceivedDataStorage*>(user_data);
  result.stream_closed = true;
  return 0;
}

// implements nghttp2_error_callback2
int CheckErrors(nghttp2_session*, int lib_error_code, const char* msg,
                size_t len, void*) {
  ADD_FAILURE() << "ERROR " << lib_error_code << ": "
                << std::string_view{msg, len};
  return 0;
}

// implements nghttp2_error_callback2
int CheckInvalidFrame(nghttp2_session*, const nghttp2_frame*,
                      int lib_error_code, void*) {
  ADD_FAILURE() << "ERROR invalid frame " << lib_error_code;
  return 0;
}

// implements nghttp2_on_invalid_header_callback
int CheckInvalidHeader(nghttp2_session*, const nghttp2_frame*,
                       const uint8_t* name, size_t namelen,
                       const uint8_t* value, size_t valuelen, uint8_t, void*) {
  ADD_FAILURE() << "ERROR invalid header "
                << std::string_view{reinterpret_cast<const char*>(name),
                                    namelen}
                << std::string_view{reinterpret_cast<const char*>(value),
                                    valuelen};
  return 0;
}

void ReceiveUsingClientSession(engine::io::Socket& client_socket,
                               ReceivedDataStorage& received,
                               const engine::Deadline& test_deadline) {
  std::vector<char> buffer(4096, '\0');
  const auto reply_size =
      client_socket.RecvAll(buffer.data(), buffer.size(), test_deadline);
  buffer.resize(reply_size);

  constexpr std::string_view kSwitchingProtocolsExpected =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Connection: Upgrade\r\n"
      "Upgrade: h2c";
  const std::string_view received_switching_protocols{
      buffer.begin(), buffer.begin() + kSwitchingProtocolsExpected.size()};
  ASSERT_EQ(received_switching_protocols, kSwitchingProtocolsExpected);
  buffer.erase(buffer.begin(),
               buffer.begin() + kSwitchingProtocolsExpected.size());

  nghttp2_session* client = nullptr;
  nghttp2_session_callbacks* callbacks = nullptr;
  int rv = nghttp2_session_callbacks_new(&callbacks);
  ASSERT_EQ(rv, 0);
  nghttp2_session_callbacks_set_on_header_callback(callbacks,
                                                   StoreReceivedHeader);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks,
                                                            StoreReceivedData);
  nghttp2_session_callbacks_set_on_stream_close_callback(callbacks,
                                                         HandleStreamClosed);
  nghttp2_session_callbacks_set_error_callback2(callbacks, CheckErrors);
  nghttp2_session_callbacks_set_on_invalid_frame_recv_callback(
      callbacks, CheckInvalidFrame);
  nghttp2_session_callbacks_set_on_invalid_header_callback(callbacks,
                                                           CheckInvalidHeader);
  rv = nghttp2_session_client_new(&client, callbacks, &received);
  ASSERT_EQ(rv, 0);
  nghttp2_session_callbacks_del(callbacks);

  std::vector<uint8_t> settings_payload;
  constexpr size_t kSettingsEntryLength = 6;
  std::vector<nghttp2_settings_entry> settings{
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
  settings_payload.resize(settings.size() * kSettingsEntryLength);
  int settings_payload_len = nghttp2_pack_settings_payload(
      settings_payload.data(), settings_payload.size(), settings.data(),
      settings.size());
  ASSERT_GT(settings_payload_len, 0);

  rv = nghttp2_session_upgrade2(client, settings_payload.data(),
                                settings_payload_len, 0, &received);
  ASSERT_EQ(rv, 0);

  rv = nghttp2_session_mem_recv(
      client, reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
  ASSERT_EQ(rv, buffer.size());

  nghttp2_session_del(client);
}

void CreateServerSession(nghttp2_session** session) {
  nghttp2_session_callbacks* callbacks = nullptr;
  int rv = nghttp2_session_callbacks_new(&callbacks);
  ASSERT_EQ(rv, 0);
  nghttp2_session_callbacks_set_send_data_callback(
      callbacks, server::http::nghttp2_callbacks::send::OnSendDataCallback);
  nghttp2_session_callbacks_set_error_callback2(callbacks, CheckErrors);
  rv = nghttp2_session_server_new(session, callbacks, nullptr);
  ASSERT_EQ(rv, 0);

  nghttp2_session_callbacks_del(callbacks);
}

void AcceptClientSettings(nghttp2_session* session,
                          const std::string_view http2_settings_header) {
  const auto settings_payload =
      crypto::base64::Base64UrlDecode(http2_settings_header);
  int rv = nghttp2_session_upgrade2(
      session, reinterpret_cast<const uint8_t*>(settings_payload.data()),
      settings_payload.size(), 0, nullptr);
  // todo: ask what to do when user send incorrect data
  ASSERT_EQ(rv, 0);
}

auto CreateSendResponseTask(server::http::HttpResponse& response,
                            engine::io::Socket&& socket,
                            nghttp2_session* session) {
  return engine::AsyncNoSpan(
      [](auto&& response, auto&& socket, nghttp2_session* session) {
        std::vector<nghttp2_settings_entry> server_settings = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
        concurrent::Variable<server::http::impl::SessionPtr> session_holder_{
            session, nghttp2_session_del};
        server::http::WriteHttp2Response(
            socket, response, session_holder_,
            server::http::Http2UpgradeData{std::cref(server_settings)});
      },
      std::ref(response), std::move(socket), session);
}

UTEST(HttpResponse, Http2Write) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  server::request::ResponseDataAccounter accounter;
  server::http::HttpRequestImpl request{accounter};
  server::http::HttpResponse response{request, accounter};

  nghttp2_session* session = nullptr;
  CreateServerSession(&session);

  // we simulate that client had connnected from HTTP/1.1 and wants to upgrade
  AcceptClientSettings(session, "AAMAAABkAAQAAP__");

  response.stream_id = 1;

  constexpr std::string_view kBody = "test data";
  response.SetData(std::string{kBody});
  response.SetStatus(server::http::HttpStatus::kOk);
  response.SetHeader("test-header", "test-value");
  response.SetHeader("date", "Sat, 18 Mar 2023 23:28:06 UTC");

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);
  auto send_task = CreateSendResponseTask(response, std::move(server), session);

  {
    ReceivedDataStorage received;
    ReceiveUsingClientSession(client, received, test_deadline);

    ReceivedDataStorage expected{
        .headers = {{":status", "200"},
                    {"content-type", "text/html; charset=utf-8"},
                    {"date", "Sat, 18 Mar 2023 23:28:06 UTC"},
                    {"test-header", "test-value"},
                    {"content-length", "9"}},
        .data = "test data",
        .stream_closed = true};
    ASSERT_EQ(received.headers, expected.headers);
    ASSERT_EQ(received.data, expected.data);
    ASSERT_EQ(received.stream_closed, expected.stream_closed);
  }
}

UTEST(HttpResponse, Http2WriteStreamedNoData) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  server::request::ResponseDataAccounter accounter;
  server::http::HttpRequestImpl request{accounter};
  server::http::HttpResponse response{request, accounter};

  nghttp2_session* session = nullptr;
  CreateServerSession(&session);

  // we simulate that client had connnected from HTTP/1.1 and wants to upgrade
  AcceptClientSettings(session, "AAMAAABkAAQAAP__");

  response.stream_id = 1;

  response.SetStatus(server::http::HttpStatus::kOk);
  response.SetHeader("test-header", "test-value");
  response.SetHeader("date", "Sat, 18 Mar 2023 23:28:06 UTC");
  response.SetStreamBody();
  response.GetBodyProducer().Release();

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);
  auto send_task = CreateSendResponseTask(response, std::move(server), session);

  {
    ReceivedDataStorage received;
    ReceiveUsingClientSession(client, received, test_deadline);

    ReceivedDataStorage expected{
        .headers = {{":status", "200"},
                    {"content-type", "text/html; charset=utf-8"},
                    {"date", "Sat, 18 Mar 2023 23:28:06 UTC"},
                    {"test-header", "test-value"}},
        .data = "",
        .stream_closed = true};
    ASSERT_EQ(received.headers, expected.headers);
    ASSERT_EQ(received.data, expected.data);
    ASSERT_EQ(received.stream_closed, expected.stream_closed);
  }
}

UTEST(HttpResponse, Http2WriteStreamedWithData) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  server::request::ResponseDataAccounter accounter;
  server::http::HttpRequestImpl request{accounter};
  server::http::HttpResponse response{request, accounter};

  nghttp2_session* session = nullptr;
  CreateServerSession(&session);

  // we simulate that client had connnected from HTTP/1.1 and wants to upgrade
  AcceptClientSettings(session, "AAMAAABkAAQAAP__");

  response.stream_id = 1;

  response.SetStatus(server::http::HttpStatus::kOk);
  response.SetHeader("test-header", "test-value");
  response.SetHeader("date", "Sat, 18 Mar 2023 23:28:06 UTC");
  response.SetStreamBody();
  {
    auto body_producer = response.GetBodyProducer();
    EXPECT_TRUE(body_producer.Push("part1"));
    EXPECT_TRUE(body_producer.Push("part2"));
  }

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);

  auto send_task = CreateSendResponseTask(response, std::move(server), session);

  ReceivedDataStorage received;
  ReceiveUsingClientSession(client, received, test_deadline);

  ReceivedDataStorage expected{
      .headers = {{":status", "200"},
                  {"content-type", "text/html; charset=utf-8"},
                  {"date", "Sat, 18 Mar 2023 23:28:06 UTC"},
                  {"test-header", "test-value"}},
      .data = "part1part2",
      .stream_closed = true};

  ASSERT_EQ(received.headers, expected.headers);
  ASSERT_EQ(received.data, expected.data);
  ASSERT_EQ(received.stream_closed, expected.stream_closed);
}

class HttpResponseBody : public testing::TestWithParam<int> {};

UTEST_P(HttpResponseBody, ForbiddenBody) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  server::request::ResponseDataAccounter accounter;
  server::http::HttpRequestImpl request{accounter};
  server::http::HttpResponse response{request, accounter};

  response.SetData("test data");
  response.SetStatus(static_cast<server::http::HttpStatus>(GetParam()));

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);
  auto send_task = engine::AsyncNoSpan(
      [](auto&& response, auto&& socket) { response.SendResponse(socket); },
      std::ref(response), std::move(server));

  std::vector<char> buffer(4096, '\0');
  const auto reply_size =
      client.RecvAll(buffer.data(), buffer.size(), test_deadline);

  std::string_view reply{buffer.data(), reply_size};
  std::string expected_header = "HTTP/1.1 " + std::to_string(GetParam()) + " ";
  ASSERT_EQ(reply.substr(0, expected_header.size()), expected_header);
  EXPECT_TRUE(reply.find(http::headers::kContentLength) ==
              std::string_view::npos);
  EXPECT_EQ(reply.substr(reply.size() - 4), "\r\n\r\n");
}

INSTANTIATE_UTEST_SUITE_P(HttpResponseForbiddenBody, HttpResponseBody,
                          testing::Values(100, 101, 150, 199, 304, 204));

USERVER_NAMESPACE_END
