#include <userver/engine/task/local_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

LocalStorage::Key LocalStorage::RegisterVariable() {
  static std::atomic<Key> max_key{0};
  return max_key++;
}

LocalStorage::DataBase::DataBase() = default;

LocalStorage::DataBase::~DataBase() = default;

LocalStorage::LocalStorage() = default;

LocalStorage::~LocalStorage() {
  // By default, boost::intrusive containers don't own their elements (nodes),
  // so we need to destroy them explicitly. The variables are destroyed
  // front-to-back, in reverse-initialization order.
  while (!data_storage_.empty()) {
    auto& node = data_storage_.front();
    data_storage_.pop_front();
    delete &node;
  }
}

void* LocalStorage::GetGeneric(Key key) noexcept {
  if (data_.size() > key) return data_[key];
  return nullptr;
}

void LocalStorage::SetGeneric(Key key, void* ptr,
                              std::unique_ptr<DataBase> node) {
  if (data_.size() <= key) {
    data_.resize(key + 1);
  }

  data_[key] = ptr;
  data_storage_.push_front(*node.release());
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
