// Benchmarks grpc_protovalidate::ValidateMessage on messages from tests/proto/types/constrained.proto,
// which carry real protovalidate rules (field rules + message-level CEL).
#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/arena.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/impl/static_registration.hpp>

#include <userver/grpc-protovalidate/validate.hpp>

#include <types/constrained.pb.h>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate {

namespace {

// Keep the original validator as an in-process baseline, with the same thread-local
// lifetime and per-call arena as the original grpc-protovalidate wrapper.
compiler::ThreadLocal kOriginalFactory = [] { return buf::validate::ValidatorFactory::New().value(); };

ValidationResult ValidateOriginal(const google::protobuf::Message& message, const ValidationParams& params) {
    auto factory = kOriginalFactory.Use();
    google::protobuf::Arena arena;
    auto validator = (*factory)->NewValidator(&arena, params.fail_fast);
    auto result = validator.Validate(message);
    if (!result.ok()) {
        return ValidationError(
            ValidationError::Type::kInternal,
            fmt::format(
                "internal protovalidate error (check constraints syntax in the proto file) - {}",
                result.status().ToString()
            ),
            message.GetTypeName()
        );
    }
    if (result->violations_size() != 0) {
        return ValidationError(result.value(), message.GetTypeName());
    }
    return {};
}

template <bool kOriginal>
void RunValidation(
    benchmark::State& state,
    const google::protobuf::Message& message,
    const ValidationParams& params = {}
) {
    // The Arcadia benchmark runner does not finalize userver's static registration.
    utils::impl::FinishStaticRegistration();
    const auto validate = [&] {
        if constexpr (kOriginal) {
            return ValidateOriginal(message, params);
        } else {
            return ValidateMessage(message, params);
        }
    };
    // Warm the selected validator on the root message before the measured loop.
    const auto warmup = validate();
    if (!warmup.IsSuccess() && warmup.GetError().GetType() == ValidationError::Type::kInternal) {
        state.SkipWithError(warmup.GetError().GetDescription());
        return;
    }
    for (auto _ : state) {
        auto result = validate();
        benchmark::DoNotOptimize(result);
    }
}

// A fully-populated message that satisfies every protovalidate rule in `ConstrainedMessage`
// (mirrors tests/utils.cpp::CreateValidMessage). `required_value + in_rule(=3) <= 10` must hold.
types::ConstrainedMessage CreateValidMessage(std::int32_t required_value) {
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

// A message that violates every rule in `ConstrainedMessage` (mirrors tests/utils.cpp::CreateInvalidMessage).
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

types::ConstrainedRequest CreateValidRequest(int messages_count) {
    types::ConstrainedRequest request;
    request.set_field(1);
    for (int i = 0; i < messages_count; ++i) {
        *request.add_messages() = CreateValidMessage(2);
    }
    return request;
}

// Same data shape as CreateValidMessage, but on a type WITHOUT protovalidate rules.
types::UnconstrainedMessage CreateUnconstrainedMessage() {
    types::UnconstrainedMessage msg;
    msg.set_required_rule(2);
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

types::UnconstrainedRequest CreateUnconstrainedRequest(int messages_count) {
    types::UnconstrainedRequest request;
    request.set_field(1);
    for (int i = 0; i < messages_count; ++i) {
        *request.add_messages() = CreateUnconstrainedMessage();
    }
    return request;
}

}  // namespace

// false selects the optimized visitor, true selects the original protovalidate traversal.
template <bool kOriginal>
void ValidateValidMessage(benchmark::State& state) {
    RunValidation<kOriginal>(state, CreateValidMessage(2));
}
BENCHMARK_TEMPLATE(ValidateValidMessage, false)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(ValidateValidMessage, true)->Unit(benchmark::kMicrosecond);

template <bool kOriginal>
void ValidateInvalidMessage(benchmark::State& state) {
    RunValidation<kOriginal>(state, CreateInvalidMessage());
}
BENCHMARK_TEMPLATE(ValidateInvalidMessage, false)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(ValidateInvalidMessage, true)->Unit(benchmark::kMicrosecond);

template <bool kOriginal>
void ValidateInvalidMessageFailFast(benchmark::State& state) {
    RunValidation<kOriginal>(state, CreateInvalidMessage(), {.fail_fast = true});
}
BENCHMARK_TEMPLATE(ValidateInvalidMessageFailFast, false)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(ValidateInvalidMessageFailFast, true)->Unit(benchmark::kMicrosecond);

// Dense case: every field of every populated ConstrainedMessage carries rules.
template <bool kOriginal>
void ValidateRepeated(benchmark::State& state) {
    const auto request = CreateValidRequest(static_cast<int>(state.range(0)));
    RunValidation<kOriginal>(state, request);
}
BENCHMARK_TEMPLATE(ValidateRepeated, false)
    ->Arg(1)
    ->Arg(8)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(ValidateRepeated, true)->Arg(1)->Arg(8)->Arg(64)->Arg(256)->Arg(1024)->Unit(benchmark::kMicrosecond);

template <bool kOriginal>
void ValidateNoRulesRepeated(benchmark::State& state) {
    const auto request = CreateUnconstrainedRequest(static_cast<int>(state.range(0)));
    RunValidation<kOriginal>(state, request);
}
BENCHMARK_TEMPLATE(ValidateNoRulesRepeated, false)
    ->Arg(1)
    ->Arg(8)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(ValidateNoRulesRepeated, true)
    ->Arg(1)
    ->Arg(8)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);

// Sparse case: exactly one annotated field, two message edges below the root.
// Grow three other populated branches while keeping the validation work fixed.
template <bool kOriginal>
void ValidateSparseNested(benchmark::State& state) {
    types::SparseRequest request;
    request.mutable_branch()->mutable_checked()->set_value(1);
    request.mutable_branch()->mutable_checked()->set_payload("checked payload");
    request.set_payload(std::string(1024, 'x'));
    for (int i = 0; i < state.range(0); ++i) {
        *request.mutable_branch()->add_noise() = CreateUnconstrainedMessage();
        *request.add_siblings() = CreateUnconstrainedMessage();
        (*request.mutable_lookup())[std::to_string(i)] = CreateUnconstrainedMessage();
    }
    RunValidation<kOriginal>(state, request);
}
BENCHMARK_TEMPLATE(ValidateSparseNested, false)
    ->Arg(1)
    ->Arg(8)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(ValidateSparseNested, true)
    ->Arg(1)
    ->Arg(8)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);

}  // namespace grpc_protovalidate

USERVER_NAMESPACE_END
