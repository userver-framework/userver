## JSON schema codegen - the Chaotic

Sometimes it is required to declare a data structure and to define parse/serialize methods for it.
It is OK if you do it manually once a month, but it becomes uncomfortable to do it manually more frequently.

Chaotic codegen allows you to define a data structure in a declarative
[JSONSchema](https://json-schema.org/understanding-json-schema/reference) form and get parsing/serialization functions
for free.

It is often used for dynamic config structure/parser generation and OpenAPI request/response body types/parsers/serializers generation.


### Running JSON schema chaotic generator

You can use chaotic in three simple steps:

1) describe your JSON schema in yaml file(s);

2) run `chaotic-gen` executable;

3) use generated .hpp and .cpp files in your C++ project.

Also we've made `chaotic-gen` cmake wrappers for your convenience.

Let's go through the list number by number.

#### Describe your JSON schema in yaml file(s)

Types must be described using [JSONSchema](https://json-schema.org/learn/getting-started-step-by-step)
declarative yaml-based language.
You choose the file layout (i.e. in what YAML subpath your types are described).

`schemas/hello.yaml`:

@include samples/chaotic_service/schemas/hello.yaml

`schemas/types.yaml`:

@include samples/chaotic_service/schemas/types.yaml

You may search for schemas in `CMakeLists.txt` with `file()` command:

```cmake
file(GLOB_RECURSE SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml)
```

@warning The root type that you are planning to parse/serialize should be a structure! Otherwize the validators
         may not work as expected and your API becomes poorly extensible.


#### Run chaotic-gen executable

Now the most interesting part comes into play. We want to generate hpp and cpp files with C++ types described in YAML file.
We use `chaotic-gen` executable for that. You may call it directly from terminal for debug purposes, but we strongly
recommend to use `userver_target_generate_chaotic()` macro in your `CMakeLists.txt`.

@snippet samples/chaotic_service/CMakeLists.txt chaotic

All parameters of `userver_target_generate_chaotic()` are described below.

| Parameter                    | Kind        | Default                       | Description                                                                                        |
|------------------------------|-------------|-------------------------------|----------------------------------------------------------------------------------------------------|
| `SCHEMAS`                    | multi-value | **required**                  | JSONSchema source files to process.                                                                |
| `RELATIVE_TO`                | one-value   | **required**                  | Root directory for computing output paths relative to schemas.                                     |
| `LAYOUT`                     | multi-value | **required**                  | Mapping of in-file YAML path regex to C++ type name. Each item is `regex=Ns::Type{0}`. The path regex is written first, then `=`, then C++ type name (may include `{0}`, `{1}`, ... capture groups, or `{stem}` for the filename stem). |
| `OUTPUT_DIR`                 | one-value   | `${CMAKE_CURRENT_BINARY_DIR}` | Directory where generated `.cpp`, `.hpp`, `.ipp` files are placed.                                 |
| `OUTPUT_PREFIX`              | one-value   | `""`                          | Subdirectory prefix appended under `OUTPUT_DIR`.                                                   |
| `GENERATE_SERIALIZERS`       | option      | off                           | Generate JSON serializers besides the JSON parser.                                                 |
| `PARSE_EXTRA_FORMATS`        | option      | off                           | Also generate YAML and YAML-config parsers besides the JSON parser.                                |
| `NO_SAX_PARSE`               | option      | off                           | Skip SAX parser generation; omits `_sax_parsers.hpp` and disables `FromJsonString()`.              |
| `ERASE_PATH_PREFIX`          | one-value   | `""`                          | Strip this prefix from `$ref` paths when resolving cross-file references.                          |
| `FORMAT`                     | one-value   | `${USERVER_CHAOTIC_FORMAT}`   | `ON` or `OFF`: whether to run clang-format on generated files.                                     |
| `INCLUDE_DIRS`               | multi-value | —                             | Extra `-I` paths used when looking up headers for `x-usrv-cpp-type` values.                        |
| `LINK_TARGETS`               | multi-value | —                             | CMake targets to link; their include directories are also passed to the generator. Required when using `x-usrv-cpp-type`. |
| `INSTALL_INCLUDES_COMPONENT` | one-value   | —                             | CPack install component for generated header files.                                                |


#### Use generated .hpp and .cpp files from JSONSchema

With the setting above `${CMAKE_CURRENT_SOURCE_DIR}/schemas/hello.yaml` will produce a set of
`schemas/hello*.[hc]pp` files inside of `${CMAKE_CURRENT_BINARY_DIR}` directory. The files are as following:

| File                    | Contents                                          |
|-------------------------|---------------------------------------------------|
| `hello.hpp`             | Type declarations                                 |
| `hello_fwd.hpp`         | Type forward declarations                         |
| `hello.cpp`             | Type-related definitions                          |
| `hello_parsers.ipp`     | Generic parsers (DOM, YAML, YAML-config)          |
| `hello_sax_parsers.hpp` | SAX parser types; also enables `FromJsonString()` |

Usually you may just include `schemas/hello.hpp` file and that's all.
If you want to reference a type without actual using it, include `schemas/hello_fwd.hpp` with type forward declaration.
If you want to use some non-standard parser (e.g. for `formats::bson::Value`), include `schemas/hello_parsers.ipp`.

For efficient zero-copy JSON parsing use the generated `FromJsonString()` free function:

```cpp
// Parses using the SAX parser (fast path); falls back to DOM if needed.
MyType obj = MyType::FromJsonString(json_string_view, formats::parse::To<MyType>{});
```

`_sax_parsers.hpp` must be visible (directly or transitively) for `FromJsonString()` to compile.
Pass `NO_SAX_PARSE` to `userver_target_generate_chaotic()` to omit this file when SAX
parsers are not needed.

The most common use-case for JSON parser/serializer is a JSON handler:

@snippet samples/chaotic_service/src/hello_service.cpp Handler


### JSONSchema types mapping to C++ types

Base JSONSchema types are mapped to C++ types as following:

| JSONSchema type | C++ type       |
|-----------------|----------------|
| `boolean`       | `bool`         |
| `number`        | `double`       |
| `integer`       | `int`          |
| `string`        | `std::string`  |
| `array`         | `std::vector`  |
| `object`        | `struct`       |
| `oneOf`         | `std::variant` |
| `allOf`         | `struct`       |
| `$ref`          | -              |


#### JSON schema type: boolean

Boolean type is mapped to C++ `bool` type.

Example:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - type boolean


#### JSON schema type: integer

| format       | C++ type       |
|--------------|----------------|
| -            | `int`          |
| `int32`      | `std::int32_t` |
| `int64`      | `std::int64_t` |

Integer supports the following validators:
* `minimum`
* `maximum`
* `minimumExclusive`
* `maximumExclusive`

Example:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - type integer

For other C++ integer types, first choose the appropriate raw type as above, then the desired C++ type:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - type integer cpp type

In this case, however, you still won't be able to fit integers greater than `INT64_MAX` into the field,
because the way it works is: first the input is parsed into `int64`, then it is cast into `std::size_t`
(with bounds checks).


#### JSON schema type: number

The number type is unconditionally mapped to C++ double type:

| format   | C++ type   |
|----------|------------|
| -        | `double`   |
| `float`  | `double`   |
| `double` | `double`   |

Number supports the following validators:
* `minimum`
* `maximum`
* `minimumExclusive`
* `maximumExclusive`

Example:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - type number


#### JSON schema type: string

String type is mapped to different C++ types:

| format                | C++ type                               |
|-----------------------|----------------------------------------|
| -                     | `std::string`                          |
| `uuid`                | `boost::uuids::uuid`                   |
| `date`                | `utils::datetime::Date`                |
| `date-time`           | `utils::datetime::TimePointTz`         |
| `date-time-iso-basic` | `utils::datetime::TimePointTzIsoBasic` |
| `date-time-fraction`  | `utils::datetime::TimePointTzIsoBasic` |
| `binary`              | `std::string`                          |
| `byte`                | `std::string`                          |
| `uri`                 | `std::string`                          |

String supports the following validators:
* `minLength`
* `maxLength`
* `pattern`

Please note: `{min,max}Length` relates to UTF-8 code points, not bytes.

Example:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - type string uuid


#### JSON schema Enums

A type with an `enum:` list is mapped to a C++ `enum class`. Chaotic generates `Parse`,
`Serialize`, `Convert`, `TryConvert`, and `operator<<` helpers automatically.

Example:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - enum

Produces:

```cpp
enum class Status {
  kPending,
  kRunning,
  kDone,
};
```

Integer enums work the same way with `type: integer` and a numeric `enum:` list.


#### JSON schema type: array

Array type is mapped to different C++ types depending on `x-usrv-cpp-container` value:

| x-usrv-cpp-container type | C++ type                |
|---------------------------|-------------------------|
| -                         | `std::vector<T>`        |
| `std::set`                | `std::set<T>`           |
| `std::unordered_set`      | `std::unordered_set<T>` |
| `C`                       | `C<T>`                  |

Array supports the following validators:
* `minItems`
* `maxItems`
* `uniqueItems`

Example:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - type array


#### JSON schema type: object

Object type produces a custom structure C++ type. Required fields of C++ type `T` produce C++ fields with the same
type `T`. Non-required fields of C++ type `T` produce C++ fields with type `std::optional<T>`.

E.g. the following JSONSchema:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - type object

produces the following C++ structure:

```cpp
struct X {
  int foo;
  std::optional<std::string> bar;
};
```

`additionalProperties` with non-false value is handled in a special way.
It adds a member `extra` which holds all non-enumerated fields.
In case of `true` it holds raw `formats::json::Value`.
In case of more specific types it holds a map of this type.
If you don't need `extra` member, you may disable its generation via `x-usrv-cpp-extra-member: false`.

You may change the container type of `extra` field with `x-usrv-cpp-extra-type`:

| x-usrv-cpp-extra-type | C++ type of extra member             |
|-----------------------|--------------------------------------|
| -                     | `std::unordered_map<std::string, T>` |
| `Custom`              | `Custom<std::string, T>`             |


Any unknown field leads to a validation failure in case of `additionalProperties: false`.
It can be overridden by setting `x-usrv-strict-parsing: false`.
In this case unknown fields will be ignored.

Use `x-usrv-cpp-name` to override the C++ field name for a property (the JSON key is unchanged):

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - object cpp name


#### JSON schema oneOf

oneOf type is mapped to C++ `std::variant<...>`.

Parsing function tries to parse input data into all variants of `oneOf` in case of no `mapping`.
It can be very time-consuming in case of huge data types, especially in case of nested `oneOf`s.
So try to use `mapping` everywhere you can to speed up the parsing.

With a `discriminator`, the parser reads a single field to select the right variant in O(1):

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - oneOf discriminator

Produces `std::variant<Circle, Rectangle>` and dispatches on the `"kind"` field value.
Integer discriminator keys are also supported.


#### JSON schema allOf

allOf is implemented using multiple inheritance of structures.
It requires that all `allOf` subcases set `additionalProperties: true`.
Due to implementation details C++ parents' `extra` is not filled during parsing.

Do not use `allOf` without a dire need!


#### JSON schema $ref

You may define a type and reference it in another part of the schema.
External references (i.e. to types defined in external files) are supported,
however cycle file dependencies are forbidden.

Cyclic references between types are forbidden.
You may not reference type B from type A and type A from type B,
otherwise A should be a part of C++ type B and vice versa.
It is possible in JSON, but not in C++.
If you still want to use such self-inclusion, you have to choose where aggregation
is changed with indirect (smart) pointer reference.
You can use `x-usrv-cpp-indirect` for that.
A type with the tag generates not `T`, but `Box<T>` type which is similar to `std::unique_ptr`,
but it can be never `nullptr`.
`nullptr` can be emulated with `std::optional<Box<T>>`.

Example:

@snippet chaotic/golden_tests/schemas/docs/docs.yaml chaotic docs - ref TreeNode

Produces the following C++ structure definition:

```cpp
namespace ns {
struct TreeNode {
  std::optional<std::string> data{};
  std::optional<USERVER_NAMESPACE::utils::Box<ns::TreeNode>> left{};
  std::optional<USERVER_NAMESPACE::utils::Box<ns::TreeNode>> right{};
};
}
```


### Direct CLI reference

`chaotic-gen` is the underlying executable invoked by `userver_target_generate_chaotic()`.
Calling it directly is useful for debugging schema parsing and output layout.

| Flag                         | Required         | Description                                                     |
|------------------------------|------------------|-----------------------------------------------------------------|
| `-n` / `--name-map`          | yes (repeatable) | `regex=Ns::Type{0}` mapping from in-file YAML path to C++ type. |
| `-o` / `--output-dir`        | yes              | Directory for generated files.                                  |
| `--relative-to`              | yes              | Root directory for computing output paths.                      |
| `file …`                     | yes              | One or more YAML/JSON schema files.                             |
| `--parse-extra-formats`      | no               | Also generate YAML and YAML-config parsers.                     |
| `--generate-serializers`     | no               | Also generate JSON serializers.                                 |
| `--no-sax-parse`             | no               | Skip SAX parser and `_sax_parsers.hpp`.                         |
| `-e` / `--erase-path-prefix` | no               | Strip prefix from `$ref` paths.                                 |
| `-I` / `--include-dir`       | no (repeatable)  | Extra include path for `x-usrv-cpp-type` header lookup.         |
| `--clang-format`             | no               | clang-format binary; set to empty string to disable formatting. |
| `-u` / `--userver`           | no               | userver namespace (default: `userver`).                         |


### JSON schema User types

One may wrap any generated type using any custom type using `x-usrv-cpp-type` tag.
The tag value is the fully qualified C++ type name you want the value to wrap into.
In case of userver's internal types you may use `userver::` namespace instead of
`USERVER_NAMESPACE`.

Chaotic looks for a header `<userver/chaotic/io/X/Y.hpp>` in all include directories
in case of `x-usrv-cpp-type: X::Y`. The header must contain:

1) the type definition;

2) `Convert` functions (see below). `Convert` function is used to transform user type into JSONSchema type and vice versa.

Pass `INCLUDE_DIRS` (or `-I` on the CLI) to `userver_target_generate_chaotic` so the generator
can find the header, and pass `LINK_TARGETS` to link with the target that provides it.

@include chaotic/integration_tests/include/userver/chaotic/io/my/custom_string.hpp


### JSON schema chaotic Parser

Parsing is implemented in two steps:

1) input data is parsed calling `Parse()` with code-generated parser type as a template parameter

2) the result is wrapped conforming to `x-usrv-cpp-type` tag value

It means that regardless of `x-usrv-cpp-type` value the whole JSONSchema validation magic
is still performed.

The whole parsing process is split into smaller steps using parsers combination.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/codegen_overview.md | @ref scripts/docs/en/userver/chaotic_dynamic_configs.md ⇨
@htmlonly </div> @endhtmlonly

@example chaotic/golden_tests/schemas/docs/docs.yaml

