#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/intrusive/slist.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class LocalStorage final {
 public:
  using Key = size_t;

  static Key RegisterVariable();

  LocalStorage();
  ~LocalStorage();

  /* Get<T> must be called with the same pair T, key
   * Otherwise it is UB.
   */
  template <typename T>
  T& Get(Key key) {
    T* old_value = static_cast<T*>(GetGeneric(key));
    if (old_value) return *old_value;

    auto new_data = std::make_unique<DataImpl<T>>();
    T& new_value = new_data->Get();
    SetGeneric(key, &new_value, std::move(new_data));
    return new_value;
  }

 private:
  using IntrusiveListBaseHook = boost::intrusive::slist_base_hook<
      boost::intrusive::link_mode<boost::intrusive::normal_link>>;

  class DataBase : public IntrusiveListBaseHook {
   public:
    DataBase();
    virtual ~DataBase();
  };

  template <typename T>
  class DataImpl final : public DataBase {
   public:
    DataImpl() = default;

    T& Get() noexcept { return variable_; }

   private:
    T variable_{};
  };

  void* GetGeneric(Key key) noexcept;

  void SetGeneric(Key key, void* ptr, std::unique_ptr<DataBase> node);

  static constexpr size_t kInitialDataSize = 4;

  boost::container::small_vector<void*, kInitialDataSize> data_;
  boost::intrusive::slist<DataBase, boost::intrusive::constant_time_size<false>>
      data_storage_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
