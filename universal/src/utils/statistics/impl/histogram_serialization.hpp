#pragma once

#include <cstddef>

#include <userver/formats/common/type.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/statistics/histogram_view.hpp>
#include <utils/statistics/impl/histogram_view_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

template <typename Value>
Value Serialize(HistogramView value, formats::serialize::To<Value>) {
    constexpr std::size_t kHistogramFieldCount = 4;
    typename Value::Builder result{formats::common::Type::kObject};
    if constexpr (requires { result.Reserve(kHistogramFieldCount); }) {
        result.Reserve(kHistogramFieldCount);
    }
    result.EmplaceNocheck("bounds", impl::histogram::Access::Bounds(value));
    result.EmplaceNocheck("buckets", impl::histogram::Access::Values(value));
    result.EmplaceNocheck("inf", value.GetValueAtInf());
    result.EmplaceNocheck("sum", value.GetTotalSum());
    return result.ExtractValue();
}

template <typename StringBuilder>
void WriteToStream(HistogramView value, StringBuilder& sw) {
    const typename StringBuilder::ObjectGuard object_guard{sw};
    sw.Key("bounds");
    WriteToStream(impl::histogram::Access::Bounds(value), sw);
    sw.Key("buckets");
    WriteToStream(impl::histogram::Access::Values(value), sw);
    sw.Key("inf");
    WriteToStream(value.GetValueAtInf(), sw);
    sw.Key("sum");
    WriteToStream(value.GetTotalSum(), sw);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
