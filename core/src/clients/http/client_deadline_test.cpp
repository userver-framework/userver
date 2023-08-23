#include <userver/utest/utest.hpp>

#include <boost/range/irange.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

using ResponseQueue = concurrent::SpmcQueue<int>;

class WaitingEchoHandler final {
 public:
  explicit WaitingEchoHandler(ResponseQueue& queue)
      : data_(std::make_shared<Data>(Data{queue.GetMultiConsumer()})) {}

  utest::HttpServerMock::HttpResponse operator()(
      const utest::HttpServerMock::HttpRequest& request) {
    int response_status{};
    const bool success = data_->responses.Pop(response_status);
    if (!success) return {500, {}, ""};

    utest::HttpServerMock::HttpResponse response{};
    response.response_status = response_status;
    response.body = request.body;
    response.headers = request.headers;
    return response;
  }

 private:
  struct Data final {
    ResponseQueue::MultiConsumer responses;
  };

  std::shared_ptr<Data> data_;
};

class HttpClientDeadline : public ::testing::Test {
 protected:
  void PushResponseCode(int code) {
    const bool success = producer_.Push(int{code});
    ASSERT_TRUE(success);
  }

  clients::http::Client& GetClient() { return *http_client_; }

  const utest::HttpServerMock& GetServer() { return http_server_; }

 private:
  const std::shared_ptr<clients::http::Client> http_client_ =
      utest::CreateHttpClient();
  const std::shared_ptr<ResponseQueue> queue_ = ResponseQueue::Create();
  const utest::HttpServerMock http_server_{WaitingEchoHandler{*queue_}};
  ResponseQueue::Producer producer_ = queue_->GetProducer();
};

void SetTaskInheritedDeadline(std::chrono::milliseconds ms) {
  server::request::TaskInheritedData data;
  data.deadline = engine::Deadline::FromDuration(ms);
  server::request::kTaskInheritedData.Set(std::move(data));
}

}  // namespace

UTEST_F(HttpClientDeadline, ZeroDeadlineIsUsedByClient) {
  SetTaskInheritedDeadline(0ms);

  auto request = GetClient()
                     .CreateRequest()
                     .get()
                     .url(GetServer().GetBaseUrl())
                     .timeout(utest::kMaxTestWaitTime);
  UEXPECT_THROW((void)request.perform(), clients::http::CancelException);
}

UTEST_F(HttpClientDeadline, DeadlineIsUsedByClient) {
  SetTaskInheritedDeadline(100ms);

  auto request = GetClient()
                     .CreateRequest()
                     .get()
                     .url(GetServer().GetBaseUrl())
                     .timeout(utest::kMaxTestWaitTime);
  UEXPECT_THROW((void)request.perform(), clients::http::CancelException);
}

UTEST_F(HttpClientDeadline, DeadlineIsUsedByClientOldConnection) {
  SetTaskInheritedDeadline(100ms);

  PushResponseCode(200);
  auto success_request = GetClient()
                             .CreateRequest()
                             .get()
                             .url(GetServer().GetBaseUrl())
                             .timeout(utest::kMaxTestWaitTime);
  UEXPECT_NO_THROW(EXPECT_TRUE(success_request.perform()->IsOk()));

  auto expired_request = GetClient()
                             .CreateRequest()
                             .get()
                             .url(GetServer().GetBaseUrl())
                             .timeout(utest::kMaxTestWaitTime);
  UEXPECT_THROW((void)expired_request.perform(),
                clients::http::CancelException);
}

UTEST_F(HttpClientDeadline, ConnectionIsReused) {
  for ([[maybe_unused]] const auto _ : boost::irange(3)) {
    PushResponseCode(200);

    const auto response = GetClient()
                              .CreateRequest()
                              .get()
                              .url(GetServer().GetBaseUrl())
                              .timeout(utest::kMaxTestWaitTime)
                              .perform();
    EXPECT_TRUE(response->IsOk());
  }

  EXPECT_EQ(GetServer().GetConnectionsOpenedCount(), 1);
}

UTEST_F(HttpClientDeadline, ConnectionIsBrokenAfterTimeout) {
  for ([[maybe_unused]] const auto _ : boost::irange(3)) {
    auto request = GetClient()
                       .CreateRequest()
                       .get()
                       .url(GetServer().GetBaseUrl())
                       .timeout(10ms);
    UEXPECT_THROW((void)request.perform(), clients::http::TimeoutException);
    engine::SleepFor(50ms);
  }

  EXPECT_EQ(GetServer().GetConnectionsOpenedCount(), 3);
}

UTEST_F(HttpClientDeadline, ConnectionIsKeptAfterDeadlineExpires) {
  constexpr auto kTimeToCleanUpConnection = 100ms;

  for ([[maybe_unused]] const auto _ : boost::irange(3)) {
    SetTaskInheritedDeadline(100ms);

    auto request = GetClient()
                       .CreateRequest()
                       .get()
                       .url(GetServer().GetBaseUrl())
                       .timeout(utest::kMaxTestWaitTime);
    UEXPECT_THROW((void)request.perform(), clients::http::CancelException);

    // Client should wait for the connection to clean up for the duration of the
    // static timeout. Server will send the response when we allow the handler
    // to complete.
    PushResponseCode(504);
    engine::SleepFor(kTimeToCleanUpConnection);
  }

  EXPECT_EQ(GetServer().GetConnectionsOpenedCount(), 1);
}

USERVER_NAMESPACE_END
