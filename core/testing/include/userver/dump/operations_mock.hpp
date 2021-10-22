#pragma once

#include <userver/dump/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// A `Writer` that appends to a string buffer (used in tests)
class MockWriter final : public Writer {
 public:
  /// Creates a `MockWriter` with an empty buffer
  MockWriter();

  void Finish() override;

  /// Extracts the built string, leaving the Writer in a valid but unspecified
  /// state
  std::string Extract() &&;

 private:
  void WriteRaw(std::string_view data) override;

  std::string data_;
};

/// A `Reader` that reads from a string buffer (used in tests)
class MockReader final : public Reader {
 public:
  /// Creates a `PipeReader` that references the given buffer
  explicit MockReader(std::string data);

  void Finish() override;

 private:
  std::string_view ReadRaw(std::size_t max_size) override;

  std::string data_;
  std::string_view unread_data_;
};

}  // namespace dump

USERVER_NAMESPACE_END
