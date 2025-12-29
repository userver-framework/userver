#include <userver/utils/shared_readable_ptr.hpp>

USERVER_NAMESPACE_BEGIN

utils::SharedReadablePtr<int> GetSharedPtr() { return std::make_shared<const int>(42); }

void test_dereference_rvalue() {
    // FAILNEXTLINE(keep the pointer before using, please)
    [[maybe_unused]] int guard = *GetSharedPtr();
}

USERVER_NAMESPACE_END
