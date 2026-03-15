#include <userver/utest/logging_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest
{

UTEST_F(LoggingFixture, DebugMessage)
{
    LOG_DEBUG() << "Test message";
}

UTEST_F(LoggingFixture, InfoMessage)
{
    LOG_INFO() << "Test message";
}

} // namespace utest

USERVER_NAMESPACE_END
