#include <userver/utest/using_namespace_userver.hpp>

/// [Sample aggregate dump]
#include <cstdint>
#include <string>

#include <userver/dump/aggregates.hpp>

namespace some_namespace {

struct MyStruct {
  std::int16_t some_field;
  std::string some_other_field;
};

}  // namespace some_namespace

// Enable dumping of the aggregate
template <>
struct dump::IsDumpedAggregate<some_namespace::MyStruct>;
/// [Sample aggregate dump]
