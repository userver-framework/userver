#include <userver/tracing/manager_component.hpp>

#include <clients/http/client_utils_test.hpp>
#include <clients/http/testsuite.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class MockTracingManager : public tracing::TracingManagerBase {
 public:
  bool TryFillSpanBuilderFromRequest(const server::http::HttpRequest&,
                                     tracing::SpanBuilder&) const override {
    ++create_new_span_counter_;
    return true;
  }

  void FillRequestWithTracingContext(
      const tracing::Span&,
      clients::http::RequestTracingEditor) const override {
    ++fill_request_counter_;
  }

  void FillResponseWithTracingContext(
      const tracing::Span&, server::http::HttpResponse&) const override {
    ++fill_response_counter_;
  }

  size_t GetCreateNewSpanCounter() const { return create_new_span_counter_; }
  size_t GetFillRequestCounter() const { return fill_request_counter_; }
  size_t GetFillResponseCounter() const { return fill_response_counter_; }

 private:
  mutable size_t create_new_span_counter_ = 0;
  mutable size_t fill_request_counter_ = 0;
  mutable size_t fill_response_counter_ = 0;
};

}  // namespace

UTEST(TracingManagerBase, TracingManagerCorrectCalls) {
  auto http_client_ptr = utest::CreateHttpClient();

  const utest::SimpleServer http_server_final{
      clients::http::Response200WithHeader{"xxx: test"}};

  const auto url = http_server_final.GetBaseUrl();
  auto& http_client = *http_client_ptr;
  std::string data{};

  MockTracingManager tracing_manager;

  const auto response = http_client.CreateRequest()
                            ->post(url, data)
                            ->timeout(std::chrono::seconds(1))
                            ->SetTracingManager(tracing_manager)
                            ->perform();

  EXPECT_TRUE(response->IsOk());
  EXPECT_EQ(tracing_manager.GetCreateNewSpanCounter(), 0);
  EXPECT_EQ(tracing_manager.GetFillRequestCounter(), 1);
  EXPECT_EQ(tracing_manager.GetFillResponseCounter(), 0);
}

USERVER_NAMESPACE_END
