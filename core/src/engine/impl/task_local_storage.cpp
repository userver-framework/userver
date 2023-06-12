#include <userver/engine/impl/task_local_storage.hpp>

#include <fmt/format.h>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <engine/task/task_context.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl::task_local {

namespace {

Key variable_count{0};

Key RegisterVariable() {
  utils::impl::AssertStaticRegistrationAllowed(
      "TaskLocalVariable registration");
  return variable_count++;
}

struct DataPtr;

using ListHook = boost::intrusive::list_member_hook<
    utils::impl::IntrusiveLinkMode, boost::intrusive::void_pointer<DataPtr*>>;

struct DataPtr final {
  DataPtr() = default;

  // ListHook lies that it's copyable. Disable copying to be safe.
  DataPtr(const DataPtr&) = delete;

  DataBase* ptr{nullptr};
  ListHook list_hook;
};

using NormalDataList = boost::intrusive::slist<
    DataPtr,
    boost::intrusive::member_hook<DataPtr, ListHook, &DataPtr::list_hook>,
    boost::intrusive::constant_time_size<false>, boost::intrusive::linear<true>,
    boost::intrusive::cache_last<false>>;

using InheritedDataList = boost::intrusive::list<
    DataPtr,
    boost::intrusive::member_hook<DataPtr, ListHook, &DataPtr::list_hook>,
    boost::intrusive::constant_time_size<false>>;

}  // namespace

DataBase::DataBase(Deleter deleter) : deleter_(deleter) {}

void DataBase::DeleteSelf() noexcept { deleter_(*this); }

void InheritedDataBase::AddRef() noexcept { ++ref_counter_; }

Key InheritedDataBase::GetKey() const noexcept { return key_; }

void ReportVariableNotSet(const std::type_info& type) {
  throw std::runtime_error(fmt::format(
      "The requested task-local variable of type '{}' has not been set",
      compiler::GetTypeName(type)));
}

struct Storage::Impl final {
  std::unique_ptr<DataPtr[]> data;
  NormalDataList normal_data_storage;
  InheritedDataList inherited_data_storage;

  void DoSetGeneric(Key key, DataBase& node);
};

Storage::Storage() { utils::impl::AssertStaticRegistrationFinished(); }

Storage::~Storage() {
  const auto disposer = [](DataPtr* node_ptr) noexcept {
    UASSERT(node_ptr->ptr);
    node_ptr->ptr->DeleteSelf();
  };

  // By default, boost::intrusive containers don't own their elements (nodes),
  // so we need to destroy them explicitly. The variables are destroyed
  // front-to-back, in reverse-initialization order.
  while (!impl_->normal_data_storage.empty()) {
    impl_->normal_data_storage.pop_front_and_dispose(disposer);
  }

  while (!impl_->inherited_data_storage.empty()) {
    impl_->inherited_data_storage.pop_front_and_dispose(disposer);
  }
}

void Storage::InheritFrom(Storage& other) {
  UASSERT(impl_->normal_data_storage.empty());
  UASSERT(impl_->inherited_data_storage.empty());

  if (!other.impl_->inherited_data_storage.empty() && !impl_->data) {
    impl_->data = std::make_unique<DataPtr[]>(variable_count);
  }

  for (DataPtr& their_ptr :
       other.impl_->inherited_data_storage | boost::adaptors::reversed) {
    UASSERT(their_ptr.ptr);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& node = static_cast<InheritedDataBase&>(*their_ptr.ptr);
    UASSERT(node.GetKey() < variable_count);

    auto& our_ptr = impl_->data[node.GetKey()];
    UASSERT(!our_ptr.ptr);
    our_ptr.ptr = &node;
    impl_->inherited_data_storage.push_front(our_ptr);
    node.AddRef();
  }
}

void Storage::InitializeFrom(Storage&& other) noexcept {
  UASSERT(impl_->normal_data_storage.empty());
  UASSERT(impl_->inherited_data_storage.empty());
  impl_ = std::move(other.impl_);
}

DataBase* Storage::GetGeneric(Key key) noexcept {
  UASSERT(key < variable_count);
  if (!impl_->data) return nullptr;
  return impl_->data[key].ptr;
}

void Storage::Impl::DoSetGeneric(Key key, DataBase& node) {
  UASSERT(key < variable_count);
  if (!data) data = std::make_unique<DataPtr[]>(variable_count);
  data[key].ptr = &node;
}

void Storage::SetGeneric(Key key, NormalDataBase& node,
                         bool has_existing_variable) {
  impl_->DoSetGeneric(key, node);
  if (!has_existing_variable) {
    impl_->normal_data_storage.push_front(impl_->data[key]);
  }
}

void Storage::SetGeneric(Key key, InheritedDataBase& node,
                         bool has_existing_variable) {
  impl_->DoSetGeneric(key, node);
  if (!has_existing_variable) {
    impl_->inherited_data_storage.push_front(impl_->data[key]);
  }
}

void Storage::EraseInherited(Key key) noexcept {
  UASSERT(key < variable_count);
  if (!impl_->data) return;

  auto& data_ptr = impl_->data[key];
  auto* const data = data_ptr.ptr;
  if (!data) return;

  data_ptr.ptr = nullptr;
  impl_->inherited_data_storage.erase(
      InheritedDataList::s_iterator_to(data_ptr));
  data->DeleteSelf();
}

Variable::Variable() : key_(RegisterVariable()) {}

Key Variable::GetKey() const noexcept { return key_; }

Storage& GetCurrentStorage() noexcept {
  return current_task::GetCurrentTaskContext().GetLocalStorage();
}

}  // namespace engine::impl::task_local

USERVER_NAMESPACE_END
