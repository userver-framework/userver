#include <userver/utils/resources.hpp>

#include <unordered_map>

#if __has_include(<library/cpp/resource/resource.h>)
#define ARCADIA
#include <library/cpp/resource/resource.h>
#endif

USERVER_NAMESPACE_BEGIN

namespace utils {

std::unordered_map<std::string_view, std::string_view>& Resources() {
    static std::unordered_map<std::string_view, std::string_view> r;
    return r;
}

void RegisterResource(std::string_view name, std::string_view value) { Resources().emplace(name, value); }

std::string FindResource(std::string_view name) {
#ifdef ARCADIA
    return NResource::Find(name);
#else
    return std::string{Resources().at(name)};
#endif
}

}  // namespace utils

USERVER_NAMESPACE_END
