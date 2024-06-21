#include <userver/utest/utest.hpp>

#include <userver/formats/json/serialize.hpp>

namespace testing {

void PrintTo(std::chrono::seconds s, std::ostream* os) {
  *os << s.count() << "s";
}

void PrintTo(std::chrono::milliseconds ms, std::ostream* os) {
  *os << ms.count() << "ms";
}

void PrintTo(std::chrono::microseconds us, std::ostream* os) {
  *os << us.count() << "us";
}

void PrintTo(std::chrono::nanoseconds ns, std::ostream* os) {
  *os << ns.count() << "ns";
}

}  // namespace testing

USERVER_NAMESPACE_BEGIN

namespace formats::json {

void PrintTo(const Value& json, std::ostream* out) { Serialize(json, *out); }

}  // namespace formats::json

USERVER_NAMESPACE_END
