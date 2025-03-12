// !TODO this @file curl-ev/mime.cpp create in 2025 year                    //

#include <curl-ev/mime.hpp>
#include <curl-ev/error_code.hpp>
#include <curl-ev/wrappers.hpp>

USERVER_NAMESPACE_BEGIN
namespace curl {

mime::mime(native::curl_mime* mime_ptr) :
    mime_(mime_ptr) {
    
}

mime::~mime() {
    if (mime_) {
        native::curl_mime_free(mime_);
        mime_ = nullptr;
    }
}

} // curl namespace
USERVER_NAMESPACE_END