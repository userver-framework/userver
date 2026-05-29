#include <storages/scylla/scylla_config.hpp>

#include <userver/dynamic_config/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace {

constexpr utils::TrivialBiMap kConsistencyMapping([](auto selector) {
    return selector()
        .Case(Consistency::kAny, "any")
        .Case(Consistency::kOne, "one")
        .Case(Consistency::kTwo, "two")
        .Case(Consistency::kThree, "three")
        .Case(Consistency::kQuorum, "quorum")
        .Case(Consistency::kAll, "all")
        .Case(Consistency::kLocalQuorum, "local_quorum")
        .Case(Consistency::kEachQuorum, "each_quorum")
        .Case(Consistency::kLocalOne, "local_one");
});

constexpr utils::TrivialBiMap kSerialConsistencyMapping([](auto selector) {
    return selector().Case(SerialConsistency::kSerial, "serial").Case(SerialConsistency::kLocalSerial, "local_serial");
});

}  // namespace

CommandControl Parse(const formats::json::Value& value, formats::parse::To<CommandControl>) {
    CommandControl result;

    if (value.HasMember("request_timeout_ms")) {
        const auto ms = value["request_timeout_ms"].As<std::int64_t>();
        if (ms <= 0) {
            throw std::runtime_error(utils::StrCat(
                "Invalid request_timeout_ms in SCYLLA_DEFAULT_COMMAND_CONTROL: "
                "must be > 0, got ",
                std::to_string(ms)
            ));
        }
        result.request_timeout = std::chrono::milliseconds{ms};
    }

    if (value.HasMember("consistency")) {
        const auto str = value["consistency"].As<std::string>();
        const auto mapped = kConsistencyMapping.TryFind(str);
        if (!mapped) {
            throw std::runtime_error(utils::StrCat(
                "Unknown consistency '",
                str,
                "' in SCYLLA_DEFAULT_COMMAND_CONTROL. Valid values: ",
                kConsistencyMapping.DescribeByType<std::string>()
            ));
        }
        result.consistency = mapped;
    }

    if (value.HasMember("serial_consistency")) {
        const auto str = value["serial_consistency"].As<std::string>();
        const auto mapped = kSerialConsistencyMapping.TryFind(str);
        if (!mapped) {
            throw std::runtime_error(utils::StrCat(
                "Unknown serial_consistency '",
                str,
                "' in SCYLLA_DEFAULT_COMMAND_CONTROL. Valid values: ",
                kSerialConsistencyMapping.DescribeByType<std::string>()
            ));
        }
        result.serial_consistency = mapped;
    }

    return result;
}

const dynamic_config::Key<utils::DefaultDict<CommandControl>> kScyllaDefaultCommandControl{
    "SCYLLA_DEFAULT_COMMAND_CONTROL",
    dynamic_config::DefaultAsJsonString{"{}"},
};

}  // namespace storages::scylla

USERVER_NAMESPACE_END
