#pragma once

#include <userver/utest/using_namespace_userver.hpp>

/// [Sample class serialization dump header]
#include <cstdint>
#include <string>
#include <vector>

#include <userver/dump/meta.hpp>  // for forward declarations and kIsDumpable

namespace dummy {

struct SampleType {
  std::vector<std::string> foo;
  std::int64_t bar{};
};

void Write(dump::Writer& writer, const SampleType& value);

SampleType Read(dump::Reader& reader, dump::To<SampleType>);

static_assert(dump::CheckDumpable<SampleType>());

}  // namespace dummy
/// [Sample class serialization dump header]
