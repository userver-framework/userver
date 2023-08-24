#include <userver/formats/bson/bson_builder.hpp>

#include <stdexcept>

#include <bson/bson.h>

#include <formats/bson/int_utils.hpp>
#include <formats/bson/value_impl.hpp>
#include <formats/bson/wrappers.hpp>
#include <userver/formats/bson/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson::impl {

BsonBuilder::BsonBuilder() = default;

BsonBuilder::BsonBuilder(const ValueImpl& value) {
  class Visitor {
   public:
    Visitor(BsonBuilder& builder) : builder_(builder) {}

    void operator()(const ParsedDocument& doc) const {
      for (const auto& [key, elem] : doc) {
        builder_.AppendInto(builder_.bson_->Get(), key, *elem);
      }
    }

    void operator()(const ParsedArray& array) const {
      ArrayIndexer indexer;
      for (const auto& elem : array) {
        builder_.AppendInto(builder_.bson_->Get(), indexer.GetKey(), *elem);
        indexer.Advance();
      }
    }

   private:
    BsonBuilder& builder_;
  };

  const auto* parsed_ptr = value.parsed_value_.load();
  if (!parsed_ptr) {
    UASSERT(!"Attempt to build a document from primitive type");
    throw std::logic_error("Attempt to build a document from primitive type");
  }

  std::visit(Visitor(*this), *parsed_ptr);
}

BsonBuilder::~BsonBuilder() = default;

BsonBuilder::BsonBuilder(const BsonBuilder&) = default;
BsonBuilder::BsonBuilder(BsonBuilder&&) noexcept = default;
BsonBuilder& BsonBuilder::operator=(const BsonBuilder&) = default;
BsonBuilder& BsonBuilder::operator=(BsonBuilder&&) noexcept = default;

BsonBuilder& BsonBuilder::Append(std::string_view key, std::nullptr_t) {
  bson_append_null(bson_->Get(), key.data(), key.size());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, bool value) {
  bson_append_bool(bson_->Get(), key.data(), key.size(), value);
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, int value) {
  bson_append_int32(bson_->Get(), key.data(), key.size(), value);
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, unsigned int value) {
  bson_append_int64(bson_->Get(), key.data(), key.size(), value);
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, long value) {
  bson_append_int64(bson_->Get(), key.data(), key.size(), value);
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, unsigned long value) {
  bson_append_int64(bson_->Get(), key.data(), key.size(), impl::ToInt64(value));
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, long long value) {
  bson_append_int64(bson_->Get(), key.data(), key.size(), value);
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key,
                                 unsigned long long value) {
  bson_append_int64(bson_->Get(), key.data(), key.size(), impl::ToInt64(value));
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, double value) {
  bson_append_double(bson_->Get(), key.data(), key.size(), value);
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, const char* value) {
  return Append(key, std::string_view(value));
}

BsonBuilder& BsonBuilder::Append(std::string_view key, std::string_view value) {
  if (!utils::text::utf8::IsValid(
          reinterpret_cast<const unsigned char*>(value.data()), value.size())) {
    throw BsonException("BSON strings must be valid UTF-8");
  }
  bson_append_utf8(bson_->Get(), key.data(), key.size(), value.data(),
                   value.size());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key,
                                 std::chrono::system_clock::time_point value) {
  int64_t ms_since_epoch =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          value.time_since_epoch())
          .count();
  bson_append_date_time(bson_->Get(), key.data(), key.size(), ms_since_epoch);
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, const Oid& value) {
  bson_append_oid(bson_->Get(), key.data(), key.size(), value.GetNative());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, const Binary& value) {
  bson_append_binary(bson_->Get(), key.data(), key.size(), BSON_SUBTYPE_BINARY,
                     value.Data(), value.Size());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key,
                                 const Decimal128& value) {
  bson_append_decimal128(bson_->Get(), key.data(), key.size(),
                         value.GetNative());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, MinKey) {
  bson_append_minkey(bson_->Get(), key.data(), key.size());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, MaxKey) {
  bson_append_maxkey(bson_->Get(), key.data(), key.size());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, const Timestamp& value) {
  bson_append_timestamp(bson_->Get(), key.data(), key.size(),
                        value.GetTimestamp(), value.GetIncrement());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, const Value& value) {
  value.impl_->CheckNotMissing();
  bson_append_value(bson_->Get(), key.data(), key.size(),
                    value.impl_->GetNative());
  return *this;
}

BsonBuilder& BsonBuilder::Append(std::string_view key, const bson_t* sub_bson) {
  bson_append_document(bson_->Get(), key.data(), key.size(), sub_bson);
  return *this;
}

void BsonBuilder::AppendInto(bson_t* dest, std::string_view key,
                             const ValueImpl& value) {
  class Visitor {
   public:
    Visitor(BsonBuilder& builder, bson_t* dest, std::string_view key)
        : builder_(builder), dest_(dest), key_(key) {}

    void operator()(const ParsedDocument& doc) const {
      SubdocBson subdoc_bson(dest_, key_.data(), key_.size());
      for (const auto& [key, elem] : doc) {
        builder_.AppendInto(subdoc_bson.Get(), key, *elem);
      }
    }

    void operator()(const ParsedArray& array) const {
      SubarrayBson subarray_bson(dest_, key_.data(), key_.size());
      ArrayIndexer indexer;
      for (const auto& elem : array) {
        builder_.AppendInto(subarray_bson.Get(), indexer.GetKey(), *elem);
        indexer.Advance();
      }
    }

   private:
    BsonBuilder& builder_;
    bson_t* dest_;
    std::string_view key_;
  };
  if (value.IsMissing()) return;

  const auto* parsed_ptr = value.parsed_value_.load();
  if (!parsed_ptr) {
    bson_append_value(dest, key.data(), key.size(), &value.bson_value_);
  } else {
    std::visit(Visitor(*this, dest, key), *parsed_ptr);
  }
}

const bson_t* BsonBuilder::Get() const { return bson_->Get(); }
bson_t* BsonBuilder::Get() { return bson_->Get(); }

BsonHolder BsonBuilder::Extract() { return bson_->Extract(); }

}  // namespace formats::bson::impl

USERVER_NAMESPACE_END
