#pragma once

/// @file userver/utils/any_storage.hpp
/// @brief @copybrief utils::AnyStorage

#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace any_storage::impl {

using Offset = std::size_t;

template <typename StorageTag>
inline Offset data_offset{0};

template <typename StorageTag>
inline std::size_t count{0};

template <typename StorageTag>
Offset RegisterData(std::size_t size, std::size_t alignment) noexcept {
  data_offset<StorageTag> +=
      (alignment - (data_offset<StorageTag> % alignment)) % alignment;
  Offset result = data_offset<StorageTag>;
  data_offset<StorageTag> += size;

  count<StorageTag> ++;
  return result;
}

void AssertStaticRegistrationAllowed();

template <typename T>
void Delete(std::byte* data) noexcept {
  reinterpret_cast<T*>(data)->~T();
}

}  // namespace any_storage::impl

template <typename StorageTag>
class AnyStorage;

template <typename StorageTag, typename Data>
class AnyStorageDataTag final {
 public:
  AnyStorageDataTag() noexcept
      : number_(any_storage::impl::count<StorageTag>),
        offset_(any_storage::impl::RegisterData<StorageTag>(sizeof(Data),
                                                            alignof(Data))) {
    static_assert(!std::is_reference_v<Data>);
    static_assert(!std::is_const_v<Data>);
    static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= alignof(Data),
                  "Overaligned data members are not supported by AnyStorage");

    any_storage::impl::AssertStaticRegistrationAllowed();
  }

 private:
  const std::size_t number_;
  const any_storage::impl::Offset offset_;

  friend class AnyStorage<StorageTag>;
};

/// @ingroup userver_containers
///
/// @brief map-like heterogeneous data storage
///
/// ## Usage example
/// @snippet utils/any_storage_test.cpp  AnyStorage
template <typename StorageTag>
class AnyStorage final {
 public:
  AnyStorage();

  ~AnyStorage();

  /// @returns Stored data.
  template <typename Data>
  const Data& Get(const AnyStorageDataTag<StorageTag, Data>& tag) const;

  /// @returns Stored data.
  /// @throws std::runtime_error if no data was stored
  template <typename Data>
  Data& Get(const AnyStorageDataTag<StorageTag, Data>& tag);

  /// @brief Stores the data.
  template <typename Data>
  Data& Set(AnyStorageDataTag<StorageTag, Data> tag, Data data);

  /// @brief Emplaces the data. The data is rewritten if
  /// already stored.
  template <typename Data, typename... Args>
  Data& Emplace(const AnyStorageDataTag<StorageTag, Data>& tag, Args&&... args);

  /// @returns Pointer to stored data or nullptr if
  /// no data is found.
  template <typename Data>
  Data* GetOptional(const AnyStorageDataTag<StorageTag, Data>& tag) noexcept;

  /// @returns Pointer to stored data or nullptr if
  /// no data found.
  template <typename Data>
  const Data* GetOptional(const AnyStorageDataTag<StorageTag, Data>& tag) const
      noexcept;

  /// @brief Erase data.
  template <typename Data>
  void Erase(const AnyStorageDataTag<StorageTag, Data>& tag);

 private:
  std::unique_ptr<std::byte[]> raw_data_;

  struct AllocRecord {
    void (*deleter)(std::byte*) noexcept;
    std::size_t offset;
  };

  AllocRecord* GetRecords() noexcept;
};

template <typename StorageTag>
AnyStorage<StorageTag>::AnyStorage()
    : raw_data_(new std::byte[any_storage::impl::data_offset<StorageTag> +
                              sizeof(AllocRecord) *
                                  any_storage::impl::count<StorageTag>]) {
  static_assert(std::is_trivial_v<AllocRecord>);

  auto records = GetRecords();
  for (std::size_t i = 0; i < any_storage::impl::count<StorageTag>; i++) {
    auto& record = records[i];
    record.deleter = nullptr;
  }
}

template <typename StorageTag>
AnyStorage<StorageTag>::~AnyStorage() {
  auto records = GetRecords();
  for (std::size_t i = 0; i < any_storage::impl::count<StorageTag>; i++) {
    auto& record = records[i];
    if (record.deleter) record.deleter(&raw_data_[record.offset]);
  }
}

template <typename StorageTag>
template <typename Data>
Data& AnyStorage<StorageTag>::Set(const AnyStorageDataTag<StorageTag, Data> tag,
                                  Data data) {
  auto number = tag.number_;
  if (!GetRecords()[number].deleter) return Emplace(tag, std::move(data));

  auto offset = tag.offset_;
  return *reinterpret_cast<Data*>(&raw_data_[offset]) = std::move(data);
}

template <typename StorageTag>
template <typename Data, typename... Args>
Data& AnyStorage<StorageTag>::Emplace(
    const AnyStorageDataTag<StorageTag, Data>& tag, Args&&... args) {
  auto number = tag.number_;
  auto& record = GetRecords()[number];
  if (record.deleter) record.deleter(&raw_data_[tag.offset_]);

  auto offset = tag.offset_;
  auto ptr = new (&raw_data_[offset]) Data(std::forward<Args>(args)...);
  record = {&any_storage::impl::Delete<Data>, offset};
  return *ptr;
}

template <typename StorageTag>
template <typename Data>
Data& AnyStorage<StorageTag>::Get(
    const AnyStorageDataTag<StorageTag, Data>& tag) {
  auto ptr = GetOptional(tag);
  if (ptr) return *ptr;
  throw std::runtime_error("No data");
}

template <typename StorageTag>
template <typename Data>
const Data& AnyStorage<StorageTag>::Get(
    const AnyStorageDataTag<StorageTag, Data>& tag) const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<AnyStorage<StorageTag>*>(this)->Get<Data>(tag);
}

template <typename StorageTag>
template <typename Data>
Data* AnyStorage<StorageTag>::GetOptional(
    const AnyStorageDataTag<StorageTag, Data>& tag) noexcept {
  auto number = tag.number_;
  auto offset = tag.offset_;
  if (!GetRecords()[number].deleter) return nullptr;
  return reinterpret_cast<Data*>(&raw_data_[offset]);
}

template <typename StorageTag>
template <typename Data>
const Data* AnyStorage<StorageTag>::GetOptional(
    const AnyStorageDataTag<StorageTag, Data>& tag) const noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<AnyStorage*>(this)->GetOptional<Data>(tag);
}

template <typename StorageTag>
typename AnyStorage<StorageTag>::AllocRecord*
AnyStorage<StorageTag>::GetRecords() noexcept {
  return reinterpret_cast<AllocRecord*>(
      &raw_data_[any_storage::impl::data_offset<StorageTag>]);
}

}  // namespace utils

USERVER_NAMESPACE_END
