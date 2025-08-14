#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils::trx_tracker {

class GlobalEnabler final {
public:
    explicit GlobalEnabler(bool enable = true);
    ~GlobalEnabler();

    GlobalEnabler(const GlobalEnabler&) = delete;
    GlobalEnabler& operator=(const GlobalEnabler&) = delete;
};

bool IsEnabled() noexcept;

}  // namespace utils::trx_tracker

USERVER_NAMESPACE_END
