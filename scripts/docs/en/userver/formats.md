## Formats (JSON, YAML, BSON, ...)

Userver provides classes for reading, working with, and serializing various
data formats. Classes for different formats have an almost identical interface.
There are common points for customizing parsers and serializers of custom types.

### formats::*::Value
Classes formats::json::Value, formats::bson::Value, yaml_config::YamlConfig
and formats::yaml::Value are intended for non-modifying work with formats
(in other words, for reading data).

Usage Example:

@snippet formats/json/value_test.cpp  Sample formats::json::Value usage


### Customization of formats::*::Value::As<T>()

In order for `formats::*::Value` to be able to represent data as a C++ type,
you should write a special function `Parse` for that C++ type. `Parse` should
be located in the namespace of the type or may be located in the 
`formats::common` namespace if the type comes from third-party library that you
have no control of:

@snippet formats/json/value_test.cpp  Sample formats::json::Value::As<T>() usage

You can write a single parser for all formats, just make it a template:

@snippet formats/common/value_test.cpp  Sample formats::*::Value::As<T>() usage


### Inline helpers formats::*::Make*

To build objects of trivial types some of the formats provide inline helpers,
like formats::json::MakeArray(), formats::json::MakeObject():

@snippet formats/json/member_access_test.cpp Sample json inline construction functions

Or formats::bson::MakeDoc(), formats::bson::MakeArray():

@snippet formats/bson/extraction_test.cpp Sample bson inline construction functions

Those inline helper functions usually work slightly faster than `formats::*::ValueBuilder`.
However, if you need a `std::string` with JSON the fastest way would be to use
the @ref formats_streaming_serialization "Streaming Serialization".
Inline helpers could not be customized
for used provided types, unlike other format building types. Inline helpers
could produce broken value on bad input because they skip some of the checks,
for example a key uniqueness check.


### formats::*::ValueBuilder

Classes `formats::json::ValueBuilder`, `formats::bson::ValueBuilder` and `formats::yaml::ValueBuilder` 
are designed for building objects of a given format.

Usage Example:

@snippet formats/json/value_builder_test.cpp  Sample formats::json::ValueBuilder usage


### Customization of formats::*::ValueBuilder
In order for `formats::*::ValueBuilder` to be able to represent a C++ type in
the specified format, you should write a special function `Serialize` for that
C++ type. `Serialize` should be located in the namespace of the type or may be
located in the `formats::common` namespace if the type comes from third-party
library that you have no control of:

@snippet formats/json/value_builder_test.cpp  Sample Customization formats::json::ValueBuilder usage

You can write a single serializer for all formats, for make it a template:

@snippet formats/common/value_builder_test.cpp  Sample Customization formats::*::ValueBuilder usage


@anchor formats_streaming_serialization
### Streaming Serialization

For runtime-critical code, it is possible to use streaming serializers. They allow you to serialize several times faster than `formats::json::ValueBuilder`, but should be used carefully because may produce broken format.


At the moment, **stream serialization is implemented only for JSON** via the `formats::json::StringBuilder`.

In order for stream serialization to work with your data type, you need to define the `WriteToStream` function in the namespace of your type:

@snippet formats/json/string_builder_test.cpp  Sample formats::json::StringBuilder usage


Note that you may get **invalid** JSON, since:
* methods `format::json methods::StringBuilder::Key` **do not** check the uniqueness of keys
* `StringBuilder` itself does not put curly brackets, you need to use formats::json::StringBuilder::ObjectGuard
* `StringBuilder` itself does not put square brackets, you need to use formats::json::StringBuilder::ArrayGuard
* You can write any nonsense in JSON using `StringBuilder` methods
* etc.

Test your serializers!


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_synchronization | @ref md_en_userver_logging ⇨
@htmlonly </div> @endhtmlonly
