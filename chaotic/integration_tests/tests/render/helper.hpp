#pragma once

#include <string>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/sax_parser.hpp>
#include <userver/formats/json/parser/exception.hpp>
#include <userver/formats/json/parser/typed_parser.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

template <typename T>
formats::json::Value TestDomSerializer(const T& value) {
    try {
        return formats::json::ValueBuilder{value}.ExtractValue();
    } catch (const std::exception& e) {
        ADD_FAILURE() << "TestDomSerializer failed: " << e.what();
        throw;
    }
}

template <typename T>
formats::json::Value TestToJsonString(const T& value) {
    std::string str;
    try {
        str = ToJsonString(value);
    } catch (const std::exception& e) {
        ADD_FAILURE() << "ToJsonString failed: " << e.what();
        throw;
    }

    try {
        return formats::json::FromString(str);
    } catch (const std::exception& e) {
        ADD_FAILURE() << "Result of ToJsonString is invalid: " << e.what() << ". ToJsonString returned: " << str;
        throw;
    }
}

template <class T>
formats::json::Value TestWriteToStream(const T& value) {
    std::string str;
    try {
        formats::json::StringBuilder builder;
        WriteToStream(value, builder);
        str = builder.GetString();
    } catch (const std::exception& e) {
        ADD_FAILURE() << "WriteToStream failed: " << e.what();
        throw;
    }

    try {
        return formats::json::FromString(str);
    } catch (const std::exception& e) {
        ADD_FAILURE() << "Result of WriteToStream is invalid: " << e.what() << ". WriteToStream returned: " << str;
        throw;
    }
}

template <typename T>
T CallSaxParser(std::string_view input) {
    using Parser = chaotic::sax::Parser<T>;
    return formats::json::parser::ParseToType<T, Parser>(input);
}

USERVER_NAMESPACE_END
