#include <userver/testsuite/testpoint.hpp>

#include <utility>
#include <variant>

#include <userver/engine/shared_mutex.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/testsuite/testpoint_control.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/function_ref.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

namespace {

using EnableOnly = utils::impl::TransparentSet<std::string>;
struct EnableAll {};
using EnabledTestpoints = std::variant<EnableOnly, EnableAll>;

std::atomic<TestpointClientBase*> client_instance{nullptr};
engine::SharedMutex client_instance_mutex;

rcu::Variable<EnabledTestpoints> enabled_testpoints;
std::atomic<TestpointControl*> control_instance{nullptr};

class TestpointScope final {
 public:
  TestpointScope() : lock_(client_instance_mutex), client_(client_instance) {}

  explicit operator bool() const noexcept { return client_ != nullptr; }

  // The returned client must only be used within Scope's lifetime
  const TestpointClientBase& GetClient() const noexcept {
    UASSERT(client_);
    return *client_;
  }

 private:
  std::shared_lock<engine::SharedMutex> lock_{};
  TestpointClientBase* client_{nullptr};
};

}  // namespace

namespace impl {

bool IsTestpointEnabled(std::string_view name) noexcept {
  if (!client_instance) return false;

  // Test facility that should not throw in production
  try {
    const auto enabled_names = enabled_testpoints.Read();
    return std::visit(utils::Overloaded{[](const EnableAll&) { return true; },
                                        [&](const EnableOnly& names) {
                                          return utils::impl::FindTransparent(
                                                     names, name) !=
                                                 names.end();
                                        }},
                      *enabled_names);
  } catch (const std::exception& e) {
    UASSERT_MSG(false, e.what());
  }

  return false;
}

void ExecuteTestpointCoro(std::string_view name,
                          const formats::json::Value& json,
                          TestpointClientBase::Callback callback) {
  TestpointScope tp_scope;
  if (!tp_scope) return;

  tp_scope.GetClient().Execute(name, json, callback);
}

void ExecuteTestpointBlocking(std::string_view name,
                              const formats::json::Value& json,
                              TestpointClientBase::Callback callback,
                              engine::TaskProcessor& task_processor) {
  auto task =
      engine::CriticalAsyncNoSpan(task_processor, ExecuteTestpointCoro, name,
                                  std::cref(json), std::cref(callback));
  task.BlockingWait();
  task.Get();
}

void DoNothing(const formats::json::Value&) noexcept {}

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
  EnableOnly enable_names;
  while (!names.empty()) {
    auto node = names.extract(names.begin());
    enable_names.insert(std::move(node.value()));
  }
  enabled_testpoints.Assign(std::move(enable_names));
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

bool AreTestpointsAvailable() noexcept {
  if (!client_instance) return false;
  return true;
}

}  // namespace testsuite

USERVER_NAMESPACE_END
