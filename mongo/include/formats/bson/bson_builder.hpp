#pragma once

#include <chrono>
#include <cstddef>

#include <formats/bson/types.hpp>
#include <formats/bson/value.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/string_view.hpp>

namespace formats::bson::impl {

class MutableBson;

class BsonBuilder {
 public:
  BsonBuilder();
  explicit BsonBuilder(const ValueImpl&);
  ~BsonBuilder();

  BsonBuilder(BsonBuilder&&) noexcept;
  BsonBuilder& operator=(BsonBuilder&&) noexcept;

  BsonBuilder& Append(utils::string_view key, std::nullptr_t);
  BsonBuilder& Append(utils::string_view key, bool);
  BsonBuilder& Append(utils::string_view key, int32_t);
  BsonBuilder& Append(utils::string_view key, int64_t);
  BsonBuilder& Append(utils::string_view key, uint64_t);
#ifdef _LIBCPP_VERSION
  BsonBuilder& Append(utils::string_view key, long);
  BsonBuilder& Append(utils::string_view key, unsigned long);
#else
  BsonBuilder& Append(utils::string_view key, long long);
  BsonBuilder& Append(utils::string_view key, unsigned long long);
#endif
  BsonBuilder& Append(utils::string_view key, double);
  BsonBuilder& Append(utils::string_view key, const char*);
  BsonBuilder& Append(utils::string_view key, utils::string_view);
  BsonBuilder& Append(utils::string_view key,
                      const std::chrono::system_clock::time_point&);
  BsonBuilder& Append(utils::string_view key, const Oid&);
  BsonBuilder& Append(utils::string_view key, const Binary&);
  BsonBuilder& Append(utils::string_view key, const Decimal128&);
  BsonBuilder& Append(utils::string_view key, MinKey);
  BsonBuilder& Append(utils::string_view key, MaxKey);

  BsonBuilder& Append(utils::string_view key, const Value&);

  const bson_t* Get() const;
  bson_t* Get();

  BsonHolder Extract();

 private:
  void AppendInto(bson_t*, utils::string_view key, const ValueImpl&);

  static constexpr size_t kSize = 8;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<MutableBson, kSize, kAlignment, true> bson_;
};

}  // namespace formats::bson::impl
