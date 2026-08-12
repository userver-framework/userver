#pragma once

#include "dynamic_config/variables/FEATURE_FLAGS.types.hpp"

#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace dynamic_config {namespace feature_flags {

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
VariableTypeRaw Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<VariableTypeRaw>)
{
    value.CheckNotMissing();
    value.CheckObjectOrNull();

    VariableTypeRaw res{
        .enable_new_logic = value["enable_new_logic"].template As<
            USERVER_NAMESPACE::chaotic::Primitive<bool>
        >(),
        .timeout_ms = value["timeout_ms"].template As<
            std::optional<USERVER_NAMESPACE::chaotic::WithType<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t, USERVER_NAMESPACE::chaotic::Minimum<::dynamic_config::feature_flags::VariableTypeRaw::kTimeout_MsMinimum>>, std::chrono::milliseconds>>
        >(),
    };

    return res;
}

}  // namespace feature_flags
}  // namespace dynamic_config

