#include <userver/utest/default_logger_fixture.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace utest
{

namespace {

using CoutLoggerFixture = CoutLoggerFixtureBase<::testing::Test>;

} // unnamed namespace

// intentionally commented: trace messages aren't processed today 
// by the fixture
// TEST_F(CoutLoggerFixture, TraceMessage)
// {
//     LOG_TRACE() << "Test trace message";
// }

TEST_F(CoutLoggerFixture, DebugMessage)
{
    LOG_DEBUG() << "Test debug message";
}

TEST_F(CoutLoggerFixture, InfoMessage)
{
    LOG_INFO() << "Test info message";
}

TEST_F(CoutLoggerFixture, WarningMessage)
{
    LOG_WARNING() << "Test warning message";
}

TEST_F(CoutLoggerFixture, ErrorMessage)
{
    LOG_ERROR() << "Test error message";
}

TEST_F(CoutLoggerFixture, CriticalMessage)
{
    LOG_CRITICAL() << "Test criticalmessage";
}

} // namespace utest

USERVER_NAMESPACE_END
