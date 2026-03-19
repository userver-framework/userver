#pragma once

/// @file userver/utest/default_logger_fixture.hpp
/// @brief @copybrief utest::DefaultLoggerFixture
/// @brief @copybrief utest::CoutLoggerFixture

#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#include <userver/logging/log.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// @brief Fixture that allows to set the default logger and manages its
/// lifetime.
template <class Base>
class DefaultLoggerFixture : public Base {
public:
    static void TearDownTestSuite() {
        Base::TearDownTestSuite();
        once_used_loggers.clear();
    }

protected:
    /// Set the default logger and postpone its destruction till the coroutine
    /// engine stops
    void SetDefaultLogger(logging::LoggerPtr new_logger) {
        UASSERT(new_logger);
        BackUpDefaultLogger();
        logging::impl::SetDefaultLoggerRef(*new_logger);

        // Logger could be used by the ev-thread, so we postpone the
        // destruction of logger for the lifetime of coroutine engine.
        once_used_loggers.emplace_back(std::move(new_logger));
    }

    /// Set the default logger level
    void SetDefaultLoggerLevel(logging::Level new_level) {
        BackUpDefaultLogger();
        logging::SetDefaultLoggerLevel(new_level);
    }

    ~DefaultLoggerFixture() override { RestoreDefaultLogger(); }

private:
    void BackUpDefaultLogger() {
        if (!logger_initial_) {
            logger_initial_ = &logging::GetDefaultLogger();
            level_initial_ = logging::GetLoggerLevel(*logger_initial_);
        }
    }

    void RestoreDefaultLogger() noexcept {
        if (logger_initial_) {
            logging::impl::SetDefaultLoggerRef(*logger_initial_);
            logging::SetLoggerLevel(*logger_initial_, level_initial_);
        }
    }

    logging::impl::LoggerBase* logger_initial_{nullptr};
    logging::Level level_initial_{};

    static inline std::vector<logging::LoggerPtr> once_used_loggers;
};

namespace detail {

class CoutLogger final : public logging::impl::TextLogger {
public:
    CoutLogger() : TextLogger(logging::Format::kTskv)
    {
        SetLevel(logging::Level::kDebug);
    }

    void Log(logging::Level level, logging::impl::formatters::LoggerItemRef item) override
    {
        UASSERT(dynamic_cast<logging::impl::TextLogItem*>(&item));
        auto& str = static_cast<logging::impl::TextLogItem&>(item);
        std::lock_guard lock(m_mutex);
        std::cout << std::this_thread::get_id()
            << "\t" << logging::ToString(level) 
            << "\t" << std::string_view(&*str.log_line.begin(), std::distance(str.log_line.begin(), str.log_line.end()))
            << std::endl;
    }
private:
    std::mutex m_mutex;
};

inline
logging::LoggerPtr MakeCoutLogger() {
    static CoutLogger g_cout;

    using logging::impl::LoggerBase;
    return std::shared_ptr<LoggerBase>(std::shared_ptr<LoggerBase>{}, &g_cout);
}

}  // namespace detail

/// @brief Fixture that provides CoutLogger as the default logger and manages its
/// lifetime (in the same way as DefaultLoggerFixture)
/// @snippet universal/utest/src/utest/default_logger_test.cpp - the fixture's usage sample
template <class Base>
class CoutLoggerFixtureBase : public DefaultLoggerFixture<Base> {
public:
    CoutLoggerFixtureBase() {
        DefaultLoggerFixture<Base>::SetDefaultLogger(detail::MakeCoutLogger());
    }
};

}  // namespace utest

USERVER_NAMESPACE_END
