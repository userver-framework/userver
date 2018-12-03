#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

#include <boost/container/small_vector.hpp>

namespace engine {

class LocalStorage {
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

    ptr = new T();
    SetGeneric(key, ptr, &LocalStorage::Deleter<T>);
    return ptr;
  }

 private:
  using DeleterType = void (*)(void*);

  void* GetGeneric(Key key);

  void* SetGeneric(Key key, void* ptr, DeleterType deleter);

  template <typename T>
  static void Deleter(void* ptr) {
    T* tptr = static_cast<T*>(ptr);
    delete tptr;
  }

  struct Data {
    ~Data() {
      if (deleter) deleter(ptr);
    }

    void* ptr = nullptr;
    DeleterType deleter = nullptr;
  };

  static constexpr size_t kInitialDataSize = 4;
  boost::container::small_vector<Data, kInitialDataSize> data_;
};

}  // namespace engine
