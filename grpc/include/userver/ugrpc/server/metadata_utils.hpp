#pragma once

/// @file userver/ugrpc/server/metadata_utils.hpp
/// @brief Utilities to work with the request metadata

#include <map>
#include <string_view>
#include <utility>

#include <grpcpp/support/string_ref.h>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/ugrpc/server/call_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Returns an std::input_range containing std::string_view
/// which are non-owning references to the values of the metadata field.
/// The references must not outlive the call object to avoid undefined behavior.
inline auto GetRepeatedMetadata(CallContextBase& context, std::string_view field_name) {
    UASSERT(field_name == utils::text::ToLower(field_name));
    const auto& metadata = context.GetServerContext().client_metadata();
    auto [it_begin, it_end] = metadata.equal_range(grpc::string_ref(field_name.data(), field_name.length()));

    using Metadata = std::multimap<grpc::string_ref, grpc::string_ref>;
    return boost::iterator_range<Metadata::const_iterator>(it_begin, it_end) |
           boost::adaptors::transformed([](const std::pair<const grpc::string_ref, grpc::string_ref>& entry) {
               return std::string_view(entry.second.begin(), entry.second.length());
           });
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
