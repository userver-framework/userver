# userver: Chaotic Codegen Functionality

This folder contains the
[JSON schema codegen functionality - userver chaotic](https://userver.tech/d8/d43/md_en_2userver_2chaotic.html)


## Implmentation details introduction

* `userver/chaotic/chaotic/` contains the program that generates C++ code from JSON schema.
* `userver/chaotic/chaotic/front/` contains the code for YAML to Python-representation transformation
* `userver/chaotic/chaotic/back/cpp` contains the code for Python-representation to C++ code transformation


## back/cpp

### DOM

DOM parsing for type `T` is customized via chaotic generated functions of form
`T Parse(Value, formats::parse::To<T>)`.

DOM serialization for type `T` is customized via chaotic generated functions of form
`Value Serialize(const T&, formats::serialize::To<Value>)`.


### SAX Parsing

SAX parsing for type `T` is customized via return type of the chaotic generated function of form
`[[maybe_unused]] SomeSaxParserType ParserOf(chaotic::sax::Type<T>)`.

The `ParserOf` functions are never used at runtime, many of those functions do not have an implementation. Such
chaotic generated functions are placed into the `*_sax_parsers.hpp` headers.

SAX requires construction of the whole SAX parser with its state at the point of parsing. In other words, the content
of all `*_sax_parsers.hpp` headers should be visible to make the parser work. To avoid compilation slowdowns the
parsers are not included in main type header. Instead a
`T FromJsonString(std::string_view json, formats::parse::To<T>)` is provided that does the SAX (or DOM)
parsing of `json`.
