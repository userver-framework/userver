#include <userver/server/request/response_base.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class TestResponse final : public server::request::ResponseBase {
public:
    using ResponseBase::ResponseBase;
    using ResponseBase::SetSent;

    bool IsBodyStreamed() const override { return false; }
    bool WaitForHeadersEnd() override { return true; }
    void SetHeadersEnd() override {}
    void SendResponse(engine::io::RwBase&) override {}
    void SetStatusServiceUnavailable() override {}
    void SetStatusOk() override {}
    void SetStatusNotFound() override {}
};

}  // namespace

TEST(ResponseDataAccounter, StartAndStopRequest) {
    server::request::ResponseDataAccounter accounter;
    const auto now = std::chrono::steady_clock::now();

    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);

    accounter.StartRequest(now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.StartRequest(now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 2);

    accounter.StopRequest(0, now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.StopRequest(0, now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(ResponseDataAccounter, ReaccountRequestKeepsCount) {
    server::request::ResponseDataAccounter accounter;
    const auto t0 = std::chrono::steady_clock::now();
    const auto t1 = t0 + std::chrono::milliseconds{5};

    accounter.StartRequest(t0);
    ASSERT_EQ(accounter.GetPendingResponsesCount(), 1);
    ASSERT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);

    accounter.ReaccountRequest(0, t0, 40, t1);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 40);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.ReaccountRequest(40, t1, 7, t1);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 7);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.StopRequest(7, t1);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(ResponseBase, AccounterStartsOnConstruction) {
    server::request::ResponseDataAccounter accounter;

    {
        const TestResponse response{accounter};
        EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
        EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);
        EXPECT_TRUE(response.GetData().empty());
    }

    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(ResponseBase, AccounterTracksSetData) {
    server::request::ResponseDataAccounter accounter;
    TestResponse response{accounter};

    const std::string body = "test data";
    response.SetData(body);

    EXPECT_EQ(response.GetData(), body);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), body.size());
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);
}

TEST(ResponseBase, AccounterSetDataReplacesPendingSize) {
    server::request::ResponseDataAccounter accounter;
    TestResponse response{accounter};

    response.SetData("hi");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 2);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    response.SetData("hello world");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 11);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    response.SetData("");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);
}

TEST(ResponseBase, AccounterStopsOnSetSent) {
    server::request::ResponseDataAccounter accounter;
    TestResponse response{accounter};

    const std::string body = "payload";
    response.SetData(body);
    ASSERT_EQ(accounter.GetPendingResponsesSizeInBytes(), body.size());
    ASSERT_EQ(accounter.GetPendingResponsesCount(), 1);

    response.SetSent(body.size(), std::chrono::steady_clock::now());

    EXPECT_TRUE(response.IsSent());
    EXPECT_EQ(response.BytesSent(), body.size());
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(ResponseBase, AccounterStopsOnSetSendFailed) {
    server::request::ResponseDataAccounter accounter;
    TestResponse response{accounter};

    response.SetData("payload");
    ASSERT_EQ(accounter.GetPendingResponsesCount(), 1);
    ASSERT_EQ(accounter.GetPendingResponsesSizeInBytes(), 7);

    response.SetSendFailed(std::chrono::steady_clock::now());

    EXPECT_TRUE(response.IsSent());
    EXPECT_EQ(response.BytesSent(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(ResponseBase, AccounterMultipleResponses) {
    server::request::ResponseDataAccounter accounter;

    auto first = std::make_unique<TestResponse>(accounter);
    first->SetData("aaa");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 3);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    auto second = std::make_unique<TestResponse>(accounter);
    second->SetData("bbbb");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 7);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 2);

    first->SetSent(3, std::chrono::steady_clock::now());
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 4);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    second.reset();
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(ResponseBase, IsLimitReached) {
    server::request::ResponseDataAccounter accounter;
    accounter.SetMaxPendingResponsesSizeInBytes(10);

    TestResponse small{accounter};
    EXPECT_FALSE(small.IsLimitReached());
    small.SetData(std::string(9, 'x'));
    EXPECT_FALSE(small.IsLimitReached());
    small.SetSent(9, std::chrono::steady_clock::now());

    TestResponse exact{accounter};
    exact.SetData(std::string(10, 'x'));
    EXPECT_TRUE(exact.IsLimitReached());
}

USERVER_NAMESPACE_END
