#include <string>
#include <string_view>

#include <userver/formats/yaml.hpp>
#include <userver/utest/utest.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/connection.hpp>
#include <ydb/impl/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

ydb::impl::TableSettings ParseTableSettings(std::string_view yaml) {
    const yaml_config::YamlConfig config{formats::yaml::FromString(std::string{yaml}), {}};
    return ydb::impl::ParseTableSettings(config, ydb::impl::secdist::DatabaseSettings{});
}

UTEST(YdbConfig, DeferredSessionCreationEnabled) {
    const auto settings = ParseTableSettings("use_deferred_session_creation: true");
    EXPECT_TRUE(settings.use_deferred_session_creation);

    const auto query_settings = ydb::impl::MakeQuerySettings(settings);
    EXPECT_TRUE(query_settings.SessionPoolSettings_.UseDeferredSessionCreation_);
}

}  // namespace

USERVER_NAMESPACE_END
