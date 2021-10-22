#include <userver/engine/task/local_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

LocalStorage::Key LocalStorage::RegisterVariable() {
  static std::atomic<Key> max_key{0};
  return max_key++;
}

void* LocalStorage::GetGeneric(Key key) {
  if (data_.size() > key) return data_[key].ptr;
  return nullptr;
}

void LocalStorage::SetGeneric(Key key, void* ptr, DeleterType deleter) {
  if (data_.size() <= key) {
    data_.resize(key + 1);
  }

  auto& data = data_[key];

  data.ptr = ptr;
  data.deleter = deleter;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
