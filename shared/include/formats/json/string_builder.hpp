#pragma once

#include <string>
#include <string_view>

#include <formats/json/string_builder_fwd.hpp>
#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>
#include <formats/serialize/to.hpp>
#include <formats/serialize/write_to_stream.hpp>
#include <utils/fast_pimpl.hpp>

namespace formats::json {

/// SAX like builder of JSON string. Use with extreme caution and only in
/// performance critical part of your code.
///
/// Prefer using WriteToStream function to add data to the StringBuilder.
class StringBuilder final : public serialize::SaxStream {
 public:
  // Required by the WriteToStream fallback to Serialize
  using Value = ::formats::json::Value;

  StringBuilder();
  ~StringBuilder();

  /// Construct this guard on new object start and its destructor will end the
  /// object
  class ObjectGuard final {
   public:
    explicit ObjectGuard(StringBuilder& sw);
    ~ObjectGuard();

   private:
    StringBuilder& sw_;
  };

  /// Construct this guard on new array start and its destructor will end the
  /// array
  class ArrayGuard final {
   public:
    explicit ArrayGuard(StringBuilder& sw);
    ~ArrayGuard();

   private:
    StringBuilder& sw_;
  };

  /// @return JSON string
  std::string GetString() const;

  void WriteNull();
  void WriteString(std::string_view value);
  void WriteBool(bool value);
  void WriteInt64(int64_t value);
  void WriteUInt64(uint64_t value);
  void WriteDouble(double value);

  /// ONLY for objects/dicts: write key
  void Key(std::string_view sw);

  /// Appends raw data
  void WriteRawString(std::string_view value);

  void WriteValue(const Value& value);

 private:
  struct Impl;
  utils::FastPimpl<Impl, 112, 8> impl_;
};

void WriteToStream(bool value, StringBuilder& sw);
void WriteToStream(int64_t value, StringBuilder& sw);
void WriteToStream(uint64_t value, StringBuilder& sw);
void WriteToStream(int value, StringBuilder& sw);
void WriteToStream(unsigned value, StringBuilder& sw);
void WriteToStream(double value, StringBuilder& sw);
void WriteToStream(const char* value, StringBuilder& sw);
void WriteToStream(std::string_view value, StringBuilder& sw);
void WriteToStream(const formats::json::Value& value, StringBuilder& sw);
void WriteToStream(const std::string& value, StringBuilder& sw);

void WriteToStream(std::chrono::system_clock::time_point tp, StringBuilder& sw);

}  // namespace formats::json
