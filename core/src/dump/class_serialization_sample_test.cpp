/// [Sample class serialization dump source]
#include "class_serialization_sample_test.hpp"  // for dummy::SampleType

#include <userver/dump/common.hpp>             // for int, string dump
#include <userver/dump/common_containers.hpp>  // for vector dump
#include <userver/dump/operations.hpp>         // for Writer/Reader

namespace dummy {

void Write(dump::Writer& writer, const SampleType& value) {
  writer.Write(value.foo);
  writer.Write(value.bar);
}

SampleType Read(dump::Reader& reader, dump::To<SampleType>) {
  SampleType value;
  value.foo = reader.Read<std::vector<std::string>>();
  value.bar = reader.Read<std::int64_t>();
  return value;
}

}  // namespace dummy
/// [Sample class serialization dump source]
