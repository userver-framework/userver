#include <userver/engine/impl/task_local_storage.hpp>

#include <ranges>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/slist.hpp>

#include <engine/task/task_context.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl::task_local {

namespace {

constinit Key variable_count{0};

std::vector<TaskInheritedVariablePriority>& InheritedVariablePriorities() noexcept {
    static std::vector<TaskInheritedVariablePriority> inherited_variable_priorities;
    return inherited_variable_priorities;
}

Key RegisterVariable(TaskInheritedVariablePriority priority) {
    utils::impl::AssertStaticRegistrationAllowed("TaskLocalVariable registration");
    UINVARIANT(
        priority < TaskInheritedVariablePriority::kNone,
        "Invalid TaskInheritedVariablePriority for a TaskInheritedVariable"
    );
    const Key key = variable_count++;
    InheritedVariablePriorities().push_back(priority);
    return key;
}

struct DataPtr;

using ListHook =
    boost::intrusive::list_member_hook<utils::impl::IntrusiveLinkMode, boost::intrusive::void_pointer<DataPtr*>>;

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
    boost::intrusive::constant_time_size<false>,
    boost::intrusive::linear<true>,
    boost::intrusive::cache_last<false>>;

using InheritedDataList = boost::intrusive::list<
    DataPtr,
    boost::intrusive::member_hook<DataPtr, ListHook, &DataPtr::list_hook>,
    boost::intrusive::constant_time_size<false>>;

}  // namespace

DataBase::DataBase(Deleter deleter)
    : deleter_(deleter)
{}

void DataBase::DeleteSelf() noexcept { deleter_(*this); }

void InheritedDataBase::AddRef() noexcept { ++ref_counter_; }

Key InheritedDataBase::GetKey() const noexcept { return key_; }

void ReportVariableNotSet(const std::type_info& type) {
    throw std::runtime_error(
        fmt::format("The requested task-local variable of type '{}' has not been set", compiler::GetTypeName(type))
    );
}

struct Storage::Impl final {
    std::unique_ptr<DataPtr[]> data;
    NormalDataList normal_data_storage;
    InheritedDataList inherited_data_storage;

    void DoSetGeneric(Key key, DataBase& node);
};

Storage::Storage() { utils::impl::AssertStaticRegistrationFinished(); }

Storage::~Storage() {
    UASSERT_MSG(
        impl_->normal_data_storage.empty(),
        "Storage::DestroyVariables must be called (inside the coroutine) before the Storage destructor if the "
        "task has any normal task-local variables. The destructors of task-local variables may sleep and access "
        "the storage, so they must not run while the Storage is being destroyed"
    );

    // Inherited variables may still be here if the task never finished normally.
    DestroyVariables();
}

void Storage::DestroyVariables() noexcept {
    const auto disposer = [](DataPtr* node_ptr) noexcept {
        UASSERT(node_ptr->ptr);
        // Unset the variable before running its destructor (POSIX
        // pthread_getspecific-style). This way GetOptional, when called from a
        // destructor of another task-local variable (they may sleep, letting
        // arbitrary engine code run), returns nullptr instead of a pointer to
        // a destroyed or currently-being-destroyed object.
        auto* const data = std::exchange(node_ptr->ptr, nullptr);
        data->DeleteSelf();
    };

    // By default, boost::intrusive containers don't own their elements (nodes),
    // so we need to destroy them explicitly. The variables are destroyed
    // front-to-back, in reverse-initialization order.
    //
    // A destructor may initialize other task-local variables. Same as for
    // `thread_local` ([basic.stc.thread]/2): a variable initialized after
    // a destructor has started executing is destroyed after that destructor
    // completes. Freshly initialized variables are pushed to the list front,
    // so the inner loops pick them up; the outer loop handles a destructor of
    // a variable of one kind initializing a variable of the other kind.
    while (!impl_->normal_data_storage.empty() || !impl_->inherited_data_storage.empty()) {
        if (!impl_->normal_data_storage.empty()) {
            impl_->normal_data_storage.pop_front_and_dispose(disposer);
        } else {
            impl_->inherited_data_storage.pop_front_and_dispose(disposer);
        }
    }
}

void Storage::InheritFrom(Storage& other, TaskInheritedVariablePriority priority) {
    UASSERT(impl_->normal_data_storage.empty());
    UASSERT(impl_->inherited_data_storage.empty());

    if (!other.impl_->inherited_data_storage.empty() && !impl_->data) {
        impl_->data = std::make_unique<DataPtr[]>(variable_count);
    }

    for (const DataPtr& their_ptr : other.impl_->inherited_data_storage | std::views::reverse) {
        UASSERT(their_ptr.ptr);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& node = static_cast<InheritedDataBase&>(*their_ptr.ptr);

        if (InheritedVariablePriorities()[node.GetKey()] < priority) {
            continue;
        }

        InheritNode(node);
    }
}

void Storage::InheritNode(InheritedDataBase& node) {
    UASSERT(node.GetKey() < variable_count);

    auto& our_ptr = impl_->data[node.GetKey()];
    UASSERT(!our_ptr.ptr);
    our_ptr.ptr = &node;
    impl_->inherited_data_storage.push_front(our_ptr);
    node.AddRef();
}

DataBase* Storage::GetGeneric(Key key) noexcept {
    UASSERT(key < variable_count);
    if (!impl_->data) {
        return nullptr;
    }
    return impl_->data[key].ptr;
}

void Storage::Impl::DoSetGeneric(Key key, DataBase& node) {
    UASSERT(key < variable_count);
    if (!data) {
        data = std::make_unique<DataPtr[]>(variable_count);
    }
    data[key].ptr = &node;
}

void Storage::SetGeneric(Key key, NormalDataBase& node, bool has_existing_variable) {
    impl_->DoSetGeneric(key, node);
    if (!has_existing_variable) {
        impl_->normal_data_storage.push_front(impl_->data[key]);
    }
}

void Storage::SetGeneric(Key key, InheritedDataBase& node, bool has_existing_variable) {
    impl_->DoSetGeneric(key, node);
    if (!has_existing_variable) {
        impl_->inherited_data_storage.push_front(impl_->data[key]);
    }
}

void Storage::EraseInherited(Key key) noexcept {
    UASSERT(key < variable_count);
    if (!impl_->data) {
        return;
    }

    auto& data_ptr = impl_->data[key];
    auto* const data = data_ptr.ptr;
    if (!data) {
        return;
    }

    data_ptr.ptr = nullptr;
    impl_->inherited_data_storage.erase(InheritedDataList::s_iterator_to(data_ptr));
    data->DeleteSelf();
}

Variable::Variable(TaskInheritedVariablePriority priority)
    : key_(RegisterVariable(priority))
{}

Key Variable::GetKey() const noexcept { return key_; }

Storage& GetCurrentStorage() noexcept { return current_task::GetCurrentTaskContext().GetLocalStorage(); }

}  // namespace engine::impl::task_local

USERVER_NAMESPACE_END
