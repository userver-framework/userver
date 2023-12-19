#pragma once

#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/statistics/histogram_view.hpp>
#include <utils/statistics/impl/histogram_view_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

template <typename Value>
Value Serialize(HistogramView value, formats::serialize::To<Value>) {
  typename Value::Builder result{formats::common::Type::kObject};
  result["bounds"] = impl::histogram::Access::Bounds(value);
  result["buckets"] = impl::histogram::Access::Values(value);
  result["inf"] = value.GetValueAtInf();
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
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
