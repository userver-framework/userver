#include <handlers/simple/sharedone/handler.hpp>

namespace handlers::simple::sharedone {

Response View::Handle(Request&&, Deps&&, USERVER_NAMESPACE::server::request::RequestContext&) { return {}; }

}  // namespace handlers::simple::sharedone
