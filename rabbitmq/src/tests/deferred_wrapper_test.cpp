#include <memory>
#include <stdexcept>
#include <string>

#include <amqpcpp.h>

#include <userver/urabbitmq/typedefs.hpp>
#include <userver/utest/utest.hpp>

#include <urabbitmq/impl/deferred_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Invoke the installed callbacks directly to exercise replies after timeout without network timing.
class DeferredGet final : public AMQP::DeferredGet {
public:
    DeferredGet() : AMQP::DeferredGet(nullptr) {}

    void Complete() {
        class Message final : public AMQP::Message {
        public:
            Message() : AMQP::Message("exchange", "key") {
                _body = "late message";
                _bodySize = 12;
            }
        } message;
        _messageCallback(message, 0, false);
    }
};

class DeferredQueue final : public AMQP::DeferredQueue {
public:
    using AMQP::Deferred::reportSuccess;

    void Complete() { reportSuccess(std::string{"late queue"}, 7, 2); }
};

UTEST(DeferredWrapper, GetReplyOutlivesTimedOutCaller) {
    auto deferred = std::make_unique<DeferredGet>();
    auto result = std::make_shared<std::string>();
    const std::weak_ptr<std::string> weak_result = result;
    auto wrapper = urabbitmq::impl::DeferredWrapper::Create();
    wrapper->WrapGet(*deferred, result);
    UEXPECT_THROW(wrapper->Wait(engine::Deadline::Passed()), std::runtime_error);
    result.reset();
    wrapper.reset();

    ASSERT_FALSE(weak_result.expired());
    deferred->Complete();
    EXPECT_EQ(*weak_result.lock(), "late message");
    deferred.reset();
    EXPECT_TRUE(weak_result.expired());
}

UTEST(DeferredWrapper, DeclareQueueReplyOutlivesTimedOutCaller) {
    auto deferred = std::make_unique<DeferredQueue>();
    auto result = std::make_shared<urabbitmq::QueueDeclareResponse>();
    const std::weak_ptr<urabbitmq::QueueDeclareResponse> weak_result = result;
    auto wrapper = urabbitmq::impl::DeferredWrapper::Create();
    wrapper->WrapDeclareQueue(*deferred, result);
    UEXPECT_THROW(wrapper->Wait(engine::Deadline::Passed()), std::runtime_error);
    result.reset();
    wrapper.reset();

    ASSERT_FALSE(weak_result.expired());
    deferred->Complete();
    const auto response = weak_result.lock();
    EXPECT_EQ(response->name, "late queue");
    EXPECT_EQ(response->message_count, 7);
    EXPECT_EQ(response->consumer_count, 2);
}

}  // namespace

USERVER_NAMESPACE_END
