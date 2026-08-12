#pragma once

#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/exceptions.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

class FieldError : public ConversionErrorBase {
private:
    enum class Type { kField = 1, kFieldBeforeIndex = 2, kRepeatedItem, kMapItem, kRepeatedIndex };

public:
    template <typename TErrorCode>
    explicit FieldError(const TErrorCode code, std::string_view description = "", std::string_view field_name = "")
        : ConversionErrorBase("This is an internal error which should not leave library APIs!"),
          code_(static_cast<int>(code)),
          description_(description)
    {
        path_.reserve(128);

        if (!field_name.empty()) {
            PrependField(field_name);
        }
    }

    template <typename TErrorCode>
    [[nodiscard]] TErrorCode GetCode() const noexcept {
        return static_cast<TErrorCode>(code_);
    }

    [[nodiscard]] const std::string& GetDescription() const noexcept { return description_; }

    [[nodiscard]] std::string GetPath() const noexcept {
        std::string path = path_;

        if (path.empty()) {
            path.append(1, '/');
        } else if (path.back() == '.') {
            path.pop_back();
        }

        return path;
    }

    FieldError& PrependField(std::string_view name) {
        if (!path_.empty() && path_.front() == '[') {
            path_.insert(0, fmt::format(fmt::runtime(GetFormatString(Type::kFieldBeforeIndex, name)), name));
        } else {
            path_.insert(0, fmt::format(fmt::runtime(GetFormatString(Type::kField, name)), name));
        }

        return *this;
    }

    FieldError& PrependRepeatedItem(std::string_view name, int index) {
        path_.insert(0, fmt::format(fmt::runtime(GetFormatString(Type::kRepeatedItem, name)), name, index));
        return *this;
    }

    FieldError& PrependMapItem(std::string_view name, std::string_view key) {
        path_.insert(0, fmt::format(fmt::runtime(GetFormatString(Type::kMapItem, name)), name, key));
        return *this;
    }

    FieldError& PrependRepeatedIndex(int index) {
        path_.insert(0, fmt::format(fmt::runtime(GetFormatString(Type::kRepeatedIndex, "")), index));
        return *this;
    }

private:
    constexpr std::string_view GetFormatString(Type type, std::string_view name) const {
        bool quoted = name.empty() || name.find('.') != std::string_view::npos;

        switch (type) {
            case Type::kField:
                return quoted ? "'{}'." : "{}.";
            case Type::kFieldBeforeIndex:
                return quoted ? "'{}'" : "{}";
            case Type::kRepeatedItem:
                return quoted ? "'{}'[{}]." : "{}[{}].";
            case Type::kMapItem:
                return quoted ? "'{}'['{}']." : "{}['{}'].";
            case Type::kRepeatedIndex:
                return "[{}].";
        }

        UINVARIANT(false, "Unknown path segment type");
    }

    int code_;
    std::string description_;
    std::string path_;
};

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
