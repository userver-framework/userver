#include "utils.hpp"

#include <userver/ugrpc/status_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

types::ConstrainedMessage CreateValidMessage(int32_t required_value) {
    types::ConstrainedMessage msg;
    msg.set_required_rule(required_value);
    msg.set_const_rule(true);
    msg.mutable_lt_rule()->set_seconds(3);
    msg.mutable_lt_rule()->set_nanos(100000);
    msg.set_gt_rule(5.7);
    msg.set_in_rule(3);
    msg.set_not_in_rule("test");
    msg.set_uuid_rule("3422b448-2460-4fd2-9183-8000de6f8343");
    msg.set_regex_rule("Hello, world!");
    msg.set_defined_only_rule(types::TEST_ENUM_VALUE_1);
    msg.add_unique_gt_rule(11);
    msg.add_unique_gt_rule(12);
    msg.add_unique_gt_rule(13);
    (*msg.mutable_keys_values_rule())["x"] = 0;
    (*msg.mutable_keys_values_rule())["xx"] = -1;
    (*msg.mutable_keys_values_rule())["xxx"] = -2;
    return msg;
}

types::ConstrainedMessage CreateInvalidMessage() {
    types::ConstrainedMessage msg;
    msg.set_required_rule(0);
    msg.set_const_rule(false);
    msg.mutable_lt_rule()->set_seconds(10);
    msg.mutable_lt_rule()->set_nanos(100000);
    msg.set_gt_rule(3.5);
    msg.set_in_rule(20);
    msg.set_not_in_rule("aaa");
    msg.set_uuid_rule("3422b448-2460-4fd2-9183");
    msg.set_regex_rule("World, hello!");
    msg.set_defined_only_rule(static_cast<types::TestEnum>(10));
    msg.add_unique_gt_rule(3);
    msg.add_unique_gt_rule(5);
    msg.add_unique_gt_rule(3);
    (*msg.mutable_keys_values_rule())["x"] = 1;
    (*msg.mutable_keys_values_rule())["xxxxx"] = -1;
    (*msg.mutable_keys_values_rule())["xxxxxx"] = 10;
    return msg;
}

std::optional<buf::validate::Violations> GetViolations(const ugrpc::client::InvalidArgumentError& err) {
    std::optional<buf::validate::Violations> result;
    auto status = ugrpc::ToGoogleRpcStatus(err.GetStatus());

    if (status) {
        for (const auto& detail : status->details()) {
            if (detail.Is<buf::validate::Violations>()) {
                result.emplace();
                detail.UnpackTo(&(*result));
            }
        }
    }

    return result;
}

}  // namespace tests

USERVER_NAMESPACE_END
