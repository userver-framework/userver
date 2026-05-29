#include <userver/formats/yaml/file.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

/// @brief Read YAML from file
formats::yaml::Value FromFile(const std::string& path)
{
    return utils::Async(
               engine::current_task::GetBlockingTaskProcessor(),
               "read-yaml-from-file",
               [&path]() { return formats::yaml::blocking::FromFile(path); }
    ).Get();
}

}  // namespace formats::yaml

USERVER_NAMESPACE_END
