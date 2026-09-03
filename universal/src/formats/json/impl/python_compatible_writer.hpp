#pragma once

#include <string_view>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {

class PythonCompatibleWriter final : public rapidjson::Writer<rapidjson::StringBuffer> {
    using Base = rapidjson::Writer<rapidjson::StringBuffer>;

public:
    explicit PythonCompatibleWriter(rapidjson::StringBuffer& buffer);

    bool Double(double value);
    bool String(const char* str, rapidjson::SizeType length, bool copy = false);
    bool Key(const char* str, rapidjson::SizeType length, bool copy = false);

private:
    bool WriteDouble(double value);
    bool WriteString(std::string_view str);
};

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
