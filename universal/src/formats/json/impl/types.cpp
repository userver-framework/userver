#include <formats/json/impl/types_impl.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {

VersionedValuePtr::Data::Data(Document&& doc)
    : Data(static_cast<Value&&>(doc)) {
  static_assert(
      // NOLINTNEXTLINE(misc-redundant-expression)
      std::is_same_v<::rapidjson::CrtAllocator, Value::AllocatorType> &&
          std::is_same_v<::rapidjson::CrtAllocator, Document::AllocatorType>,
      "Both Document and Value must use CrtAllocator for the fast move");
}

VersionedValuePtr::VersionedValuePtr() noexcept = default;

VersionedValuePtr::VersionedValuePtr(std::shared_ptr<Data>&& data) noexcept
    : data_(std::move(data)) {}

VersionedValuePtr::~VersionedValuePtr() = default;

VersionedValuePtr::operator bool() const { return !!data_; }

bool VersionedValuePtr::IsUnique() const { return data_.use_count() == 1; }

const Value* VersionedValuePtr::Get() const {
  return data_ ? &data_->native : nullptr;
}

Value* VersionedValuePtr::Get() { return data_ ? &data_->native : nullptr; }

const Value& VersionedValuePtr::operator*() const {
  UASSERT(Get());
  return *Get();
}

Value& VersionedValuePtr::operator*() {
  UASSERT(Get());
  return *Get();
}

const Value* VersionedValuePtr::operator->() const { return Get(); }

Value* VersionedValuePtr::operator->() { return Get(); }

size_t VersionedValuePtr::Version() const {
  return data_ ? data_->version.load() : kInvalidVersion;
}

void VersionedValuePtr::BumpVersion() { ++data_->version; }

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
