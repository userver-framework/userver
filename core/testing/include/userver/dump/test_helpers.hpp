#pragma once

#include <utility>

#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <userver/dump/operations_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// Converts to binary using `Write(Writer&, const T&)`
template <typename T>
std::string ToBinary(const T& value) {
  MockWriter writer;
  writer.Write(value);
  return std::move(writer).Extract();
}

/// Converts from binary using `Read(Reader&, To<T>)`
template <typename T>
T FromBinary(std::string data) {
  MockReader reader(std::move(data));
  T value = reader.Read<T>();
  reader.Finish();
  return value;
}

/// Write a value to a cache dump and immediately read it back. If `Write` and
/// `Read` are implemented correctly, the result should be equal to the original
/// value.
template <typename T>
void TestWriteReadCycle(const T& original) {
  MockReader reader(dump::ToBinary(original));
  T after_cycle = reader.Read<T>();
  reader.Finish();
  EXPECT_EQ(after_cycle, original);
}

}  // namespace dump

USERVER_NAMESPACE_END
