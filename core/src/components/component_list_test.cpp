#include <components/component_list_test.hpp>

#include <userver/formats/common/merge.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

std::string MergeYaml(std::string_view source, std::string_view patch) {
    formats::yaml::ValueBuilder merged_config{formats::yaml::FromString(std::string{source})};
    formats::common::Merge(merged_config, formats::yaml::FromString(std::string{patch}));
    return ToString(merged_config.ExtractValue());
}

}  // namespace tests

USERVER_NAMESPACE_END
