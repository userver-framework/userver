#include <userver/testsuite/testpoint.hpp>

#include <utility>
#include <variant>

#include <userver/engine/shared_mutex.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/testsuite/testpoint_control.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

namespace {

using EnableOnly = std::unordered_set<std::string>;
struct EnableAll {};
using EnabledTestpoints = std::variant<EnableOnly, EnableAll>;

std::atomic<TestpointClientBase*> client_instance{nullptr};
engine::SharedMutex client_instance_mutex;

rcu::Variable<EnabledTestpoints> enabled_testpoints;
std::atomic<TestpointControl*> control_instance{nullptr};

}  // namespace

namespace impl {

struct TestpointScope::Impl final {
  Impl() : lock(client_instance_mutex), client(client_instance) {}

  std::shared_lock<engine::SharedMutex> lock{};
  TestpointClientBase* client{nullptr};
};

TestpointScope::TestpointScope() = default;

TestpointScope::~TestpointScope() = default;

TestpointScope::operator bool() const noexcept {
  return impl_->client != nullptr;
}

const TestpointClientBase& TestpointScope::GetClient() const noexcept {
  UASSERT(impl_->client);
  return *impl_->client;
}

bool IsTestpointEnabled(const std::string& name) {
  if (!client_instance) return false;
  const auto enabled_names = enabled_testpoints.Read();
  return std::visit(utils::Overloaded{[](const EnableAll&) { return true; },
                                      [&](const EnableOnly& names) {
                                        return names.count(name) > 0;
                                      }},
                    *enabled_names);
}

void ExecuteTestpointBlocking(
    const std::string& name, const formats::json::Value& json,
    const std::function<void(const formats::json::Value&)>& callback,
    engine::TaskProcessor& task_processor) {
  engine::CriticalAsyncNoSpan(task_processor, [&] {
    TestpointScope tp_scope;
    if (!tp_scope) return;
    tp_scope.GetClient().Execute(name, json, callback);
  }).BlockingWait();
}

}  // namespace impl

TestpointClientBase::~TestpointClientBase() {
  UASSERT_MSG(!client_instance,
              "Derived classes of TestpointClientBase must call Unregister in "
              "destructor");
}

void TestpointClientBase::Unregister() noexcept {
  std::lock_guard lock(client_instance_mutex);
  TestpointClientBase* expected = this;
  client_instance.compare_exchange_strong(expected, nullptr);
}

TestpointControl::TestpointControl() {
  {
    TestpointControl* expected = nullptr;
    UINVARIANT(control_instance.compare_exchange_strong(expected, this),
               "Only 1 TestpointControl instance may exist at a time");
  }
  enabled_testpoints.Assign(EnableOnly{});
}

TestpointControl::~TestpointControl() {
  enabled_testpoints.Assign(EnableOnly{});
  control_instance = nullptr;
}

void TestpointControl::SetEnabledNames(std::unordered_set<std::string> names) {
  (void)this;  // silence clang-tidy
  enabled_testpoints.Assign(EnableOnly{std::move(names)});
}

void TestpointControl::SetAllEnabled() {
  (void)this;  // silence clang-tidy
  enabled_testpoints.Assign(EnableAll{});
}

void TestpointControl::SetClient(TestpointClientBase& client) {
  (void)this;  // silence clang-tidy
  std::lock_guard lock(client_instance_mutex);
  UINVARIANT(!client_instance,
             "Only 1 TestpointClientBase may be registered at a time");
  client_instance = &client;
}

}  // namespace testsuite

USERVER_NAMESPACE_END
