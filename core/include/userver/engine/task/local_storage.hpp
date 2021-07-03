#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <boost/container/small_vector.hpp>

namespace engine::impl {

class LocalStorage final {
 public:
  using Key = size_t;

  static Key RegisterVariable();

  /* Get<T> must be called with the same pair T, key
   * Otherwise it is UB.
   */
  template <typename T>
  T* Get(Key key) {
    T* ptr = static_cast<T*>(GetGeneric(key));
    if (ptr) return ptr;

    auto new_ptr = std::make_unique<T>();
    SetGeneric(key, new_ptr.get(), &LocalStorage::Deleter<T>);
    return new_ptr.release();
  }

 private:
  using DeleterType = void (*)(void*);

  void* GetGeneric(Key key);

  void SetGeneric(Key key, void* ptr, DeleterType deleter);

  template <typename T>
  static void Deleter(void* ptr) {
    T* tptr = static_cast<T*>(ptr);
    delete tptr;
  }

  struct Data {
    Data() = default;
    Data(Data&& other) noexcept { *this = std::move(other); }
    ~Data() {
      if (deleter) deleter(ptr);
    }

    Data& operator=(Data&& other) noexcept {
      if (this == &other) return *this;
      std::swap(other.ptr, ptr);
      std::swap(other.deleter, deleter);
      return *this;
    }

    void* ptr = nullptr;
    DeleterType deleter = nullptr;
  };

  static constexpr size_t kInitialDataSize = 4;
  boost::container::small_vector<Data, kInitialDataSize> data_;
};

}  // namespace engine::impl
