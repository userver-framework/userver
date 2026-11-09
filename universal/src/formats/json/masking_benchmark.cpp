#include <userver/formats/json/masking.hpp>

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string_view>

#include <fmt/format.h>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

namespace {

void StreamingJsonMasking(
    benchmark::State& state,
    std::string_view shape,
    std::string_view list_mode,
    std::string_view output_limit
) {
    const auto count = state.range(0);
    const auto mode =
        list_mode == "whitelist"
            ? formats::json::MaskingListMode::kWhitelist
            : formats::json::MaskingListMode::kBlacklist;
    const auto limit = output_limit == "128_bytes" ? std::size_t{128} : std::numeric_limits<std::size_t>::max();

    std::string input;
    std::vector<formats::json::MaskingPath> paths;
    if (shape == "array") {
        input = "[";
        for (int i = 0; i < count; ++i) {
            if (i) {
                input += ',';
            }
            input += R"({"id":42,"token":"sensitive","data":[1,2,3]})";
        }
        input += ']';
        paths =
            mode == formats::json::MaskingListMode::kBlacklist
                ? std::vector<formats::json::MaskingPath>{{"token"}}
                : std::vector<formats::json::MaskingPath>{{"id"}, {"data"}};
    } else if (shape == "wide_object") {
        input = "{";
        for (int i = 0; i < count; ++i) {
            if (i) {
                input += ',';
            }
            input += fmt::format(R"("field{}":42)", i);
        }
        input += '}';
        paths = {{"field0"}};
    } else if (shape == "deep_object") {
        for (int i = 0; i < count; ++i) {
            input += R"({"data":)";
        }
        input += R"({"token":"sensitive","id":42})";
        input.append(count, '}');
        formats::json::MaskingPath path(count, "data");
        path.push_back(mode == formats::json::MaskingListMode::kBlacklist ? "token" : "id");
        paths.push_back(std::move(path));
    } else {
        input = fmt::format(R"({{"token":"{}","id":42}})", std::string(count, 's'));
        const auto* selected_field = mode == formats::json::MaskingListMode::kBlacklist ? "token" : "id";
        if (shape == "large_visible_token") {
            selected_field = mode == formats::json::MaskingListMode::kBlacklist ? "id" : "token";
        }
        paths = {{selected_field}};
    }
    const formats::json::MaskingRules rules{paths, mode};

    for (auto _ : state) {
        auto result = formats::json::MaskJson(input, rules, limit);
        benchmark::DoNotOptimize(result);
    }
}

void RegisterMaskingBenchmark(
    std::string_view shape,
    std::int64_t size,
    std::string_view list_mode,
    std::string_view output_limit
) {
    benchmark::RegisterBenchmark(
        fmt::format("StreamingJsonMasking/{}/{}/{}", shape, list_mode, output_limit).c_str(),
        StreamingJsonMasking,
        shape,
        list_mode,
        output_limit
    )
        ->Arg(size)
        ->ArgName("size");
}

[[maybe_unused]] const bool kMaskingBenchmarksRegistered = [] {
    for (const auto* shape : {"array", "wide_object"}) {
        for (const auto size : {1, 100, 10000}) {
            for (const auto* list_mode : {"blacklist", "whitelist"}) {
                for (const auto* output_limit : {"unlimited", "128_bytes"}) {
                    RegisterMaskingBenchmark(shape, size, list_mode, output_limit);
                }
            }
        }
    }
    RegisterMaskingBenchmark("deep_object", 100, "blacklist", "unlimited");
    RegisterMaskingBenchmark("deep_object", 100, "whitelist", "unlimited");
    RegisterMaskingBenchmark("deep_object", 100, "blacklist", "128_bytes");
    RegisterMaskingBenchmark("large_token", 1000000, "blacklist", "unlimited");
    RegisterMaskingBenchmark("large_token", 1000000, "whitelist", "128_bytes");
    for (const auto* list_mode : {"blacklist", "whitelist"}) {
        for (const auto* output_limit : {"unlimited", "128_bytes"}) {
            RegisterMaskingBenchmark("large_visible_token", 1000000, list_mode, output_limit);
        }
    }
    return true;
}();

}  // namespace

USERVER_NAMESPACE_END
