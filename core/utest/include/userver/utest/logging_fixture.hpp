#pragma once

#include <userver/utest/utest.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/logging/impl/logger_base.hpp>

#include <atomic>
#include <concepts>
#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace detail {

class CoutLogger final : public logging::impl::TextLogger {
public:
    CoutLogger() : TextLogger(logging::Format::kTskv)
    {
#if 0
		// Actually in the case we see some manipulation with the level and in the end it doesn't work expected way
        SetLevel(logging::GetDefaultLoggerLevel()); // consider gtest_hooks.hpp usage
#else
		SetLevel(logging::Level::kDebug);
#endif
    }

    void Log(logging::Level level, logging::impl::formatters::LoggerItemRef item) override
    {
        UASSERT(dynamic_cast<logging::impl::TextLogItem*>(&item));
        auto& str = static_cast<logging::impl::TextLogItem&>(item);
        std::lock_guard lock(m_mutex);
        std::cout << std::this_thread::get_id()
            << "\t" << logging::ToString(level) 
            << "\t" << std::string_view(str.log_line.begin(), str.log_line.end())
            << std::endl;
    }
private:
    std::mutex m_mutex;
};

class LoggingFixtureEnvironment
{
    CoutLogger m_logger;

    static auto& NonOwningNullLogger() noexcept
    {
        static std::atomic<logging::impl::LoggerBase*> nullLoggerPtr(&logging::GetNullLogger());
	return nullLoggerPtr;
    }

public:
    LoggingFixtureEnvironment()
    {
        logging::impl::SetDefaultLoggerRef(m_logger); 
    }

    ~LoggingFixtureEnvironment()
    {
        logging::impl::SetDefaultLoggerRef(*NonOwningNullLogger());
    }
};

}  // namespace detail

/// @ingroup userver_utest
///
/// @brief Fixture for logger usage due to unit tests
struct LoggingFixture : ::testing::Test, detail::LoggingFixtureEnvironment {};

template<typename DerivedEnvironment>
struct LoggingFixtureEx : ::testing::Test, DerivedEnvironment
{
    static_assert(std::derived_from<DerivedEnvironment, detail::LoggingFixtureEnvironment>);
};
    
}  // namespace utest

USERVER_NAMESPACE_END

