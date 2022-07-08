#pragma once

#include <array>
#include <memory>
#include <string_view>

#include <bson/bson.h>

#include <userver/formats/bson/exception.hpp>
#include <userver/formats/bson/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson::impl {

struct RawDeleter {
  template <typename T>
  void operator()(T* ptr) const noexcept {
    bson_free(reinterpret_cast<void*>(ptr));
  }
};

template <typename T>
using RawPtr = std::unique_ptr<T, RawDeleter>;

struct DocumentDeleter {
  void operator()(bson_t* bson) const noexcept { bson_destroy(bson); }
};

class MutableBson {
 public:
  MutableBson() : bson_(bson_new()) {}

  MutableBson(const uint8_t* data, size_t len)
      : bson_(bson_new_from_data(data, len)) {}

  MutableBson(const MutableBson& other) { *this = other; }
  MutableBson(MutableBson&&) noexcept = default;

  MutableBson& operator=(const MutableBson& rhs) {
    if (this == &rhs) return *this;

    return *this = CopyNative(rhs.bson_.get());
  }

  MutableBson& operator=(MutableBson&&) noexcept = default;

  static MutableBson AdoptNative(bson_t* bson) { return MutableBson(bson); }

  static MutableBson CopyNative(const bson_t* bson) {
    return MutableBson(bson_copy(bson));
  }

  const bson_t* Get() const { return bson_.get(); }
  bson_t* Get() { return bson_.get(); }

  BsonHolder Extract() { return std::move(bson_); }

 private:
  explicit MutableBson(bson_t* bson) : bson_(bson) {}

  std::unique_ptr<bson_t, DocumentDeleter> bson_;
};

// This MUST be initialized externally
class UninitializedBson {
 public:
  UninitializedBson()
      : bson_(static_cast<bson_t*>(bson_malloc(sizeof(bson_t)))) {}

  ~UninitializedBson() {
    bson_t* native_bson_ptr = bson_.get();
    if (bson_) DocumentDeleter{}(native_bson_ptr);
  }

  bson_t* Get() { return bson_.get(); }

  BsonHolder Extract() {
    return BsonHolder(bson_.release(), [](bson_t* bson) {
      DocumentDeleter{}(bson);
      RawDeleter{}(bson);
    });
  }

 private:
  RawPtr<bson_t> bson_;
};

class ArrayIndexer {
 public:
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  ArrayIndexer(uint32_t init_idx = 0) : idx_(init_idx) {}

  uint32_t Index() const { return idx_; }

  std::string_view GetKey() {
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
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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

USERVER_NAMESPACE_END
