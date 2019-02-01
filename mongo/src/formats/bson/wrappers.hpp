#pragma once

#include <array>
#include <memory>

#include <bson/bson.h>

#include <formats/bson/exception.hpp>
#include <formats/bson/types.hpp>
#include <utils/string_view.hpp>

namespace formats::bson::impl {

struct DocumentDeleter {
  void operator()(bson_t* bson) const noexcept { bson_destroy(bson); }
};

class MutableBson {
 public:
  MutableBson() : bson_(bson_new()) {}
  MutableBson(const uint8_t* data, size_t len)
      : bson_(bson_new_from_data(data, len)) {}

  bson_t* Get() { return bson_.get(); }

  BsonHolder Extract() { return BsonHolder(std::move(bson_)); }

 private:
  std::unique_ptr<bson_t, DocumentDeleter> bson_;
};

class ArrayIndexer {
 public:
  ArrayIndexer(uint32_t init_idx = 0) : idx_(init_idx) {}

  uint32_t Index() const { return idx_; }

  utils::string_view GetKey() {
    const char* key = nullptr;
    const auto len =
        bson_uint32_to_string(idx_, &key, buf_.data(), buf_.size());
    return {key, len};
  }

  void Advance() { ++idx_; }

 private:
  static constexpr size_t kMaxStrIndexSize = 16;
  uint32_t idx_;
  std::array<char, kMaxStrIndexSize> buf_;
};

class SubarrayBson {
 public:
  SubarrayBson(bson_t* parent, const char* key, size_t key_len)
      : parent_(parent) {
    bson_append_array_begin(parent_, key, key_len, &bson_);
  }

  ~SubarrayBson() {
    bson_append_array_end(parent_, &bson_);
    bson_destroy(&bson_);
  }

  bson_t* Get() { return &bson_; }

 private:
  bson_t* const parent_;
  bson_t bson_;
};

class SubdocBson {
 public:
  SubdocBson(bson_t* parent, const char* key, size_t key_len)
      : parent_(parent) {
    bson_append_document_begin(parent_, key, key_len, &bson_);
  }

  ~SubdocBson() {
    bson_append_document_end(parent_, &bson_);
    bson_destroy(&bson_);
  }

  bson_t* Get() { return &bson_; }

 private:
  bson_t* const parent_;
  bson_t bson_;
};

}  // namespace formats::bson::impl
