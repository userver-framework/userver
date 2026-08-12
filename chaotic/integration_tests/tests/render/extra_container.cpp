#include <userver/utest/assert_macros.hpp>

#include <map>
#include <string>

#include <schemas/extra_container.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Simple, ExtraType) {
    static_assert(std::is_same_v<
                  decltype(std::declval<ns::ObjectWithExtraType>().extra),
                  std::map<std::string, std::string>>);
}

USERVER_NAMESPACE_END
