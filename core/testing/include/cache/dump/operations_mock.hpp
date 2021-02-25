#pragma once

#include <cache/dump/operations.hpp>

namespace cache::dump {

/// A `Writer` that appends to a string buffer (used in tests)
class MockWriter final : public Writer {
 public:
  /// Creates a `MockWriter` with an empty buffer
  MockWriter();

  void Finish() override;

  /// Extracts the built string, leaving the Writer in a valid but unspecified
  /// state
  std::string Extract() &&;

  void WriteRaw(std::string_view data) override;
  using Writer::Write;

 private:
  std::string data_;
};

/// A `Reader` that reads from a string buffer (used in tests)
class MockReader final : public Reader {
 public:
  /// Creates a `PipeReader` that references the given buffer
  explicit MockReader(std::string data);

  /// @brief Must be called once all data has been read
  /// @throws `Error` if there is leftover data
  void Finish() override;

  std::string_view ReadRaw(std::size_t size) override;
  using Reader::Read;

 private:
  std::string data_;
  std::string_view unread_data_;
};

}  // namespace cache::dump
