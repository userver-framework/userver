## Chaotic codegen

Sometimes it is required to declare a data structure and to define parse/serialize methods for it.
It is OK if you do it manually once a month, but it becomes uncomfortable to do it manually more frequently.

Chaotic codegen allows you to define a data structure in a declarative [JSONSchema](https://json-schema.org/understanding-json-schema/reference) form and get
parsing/serialization functions for free.
It is often used for dynamic config structure/parser generation and OpenAPI request/response body types/parsers/serializers generation.


### Running chaotic generator

You can use chaotic in three simple steps:

1) describe your JSON schema in yaml file(s);

2) run `chaotic-gen` executable;

3) use generated .hpp and .cpp files in your C++ project.

Also we've made `chaotic-gen` cmake wrappers for your convinience.

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

#### Run chaotic-gen executable

Now the most interesting part comes into play. We want to generate hpp and cpp files with C++ types described in YAML file.
We use `chaotic-gen` executable for that. You may call it directly from terminal for debug purposes, but we strongly recommend to use `userver_target_generate_chaotic()` macro in your `CMakeLists.txt`.

@snippet samples/chaotic_service/CMakeLists.txt chaotic

Some frequently used parameters are described below.

* `-n` defines types mapping from in-yaml object path to C++ type name (with namespace).
  The path regex is written first, then equal sign `=`, then C++ type name.
  `-n` can be passed multiple times.
* `-f` defines file mapping from yaml filenames to C++ filenames (exluding the extentions).
  Usually as-is mapping is used.
* `--parse-extra-formats` generates YAML and YAML config parsers besides JSON parser.
* `--generate-serializers` generates serializers into JSON besides JSON parser from `formats::json::Value`.

#### Use generated .hpp and .cpp files in your C++ project.

With the setting above `${CMAKE_CURRENT_SOURCE_DIR}/schemas/hello.yaml` will produce a set of
`schemas/hello*.[hc]pp` files inside of `${CMAKE_CURRENT_BINARY_DIR}` directory. The files are as following:

* `hello.hpp` contains types definitions
* `hello_fwd.hpp` contains types forward declarations
* `hello.cpp` contains types-related definitions
* `hello_parsers.ipp` contains types generic parsers

Usually you may just include `schemas/hello.hpp` file and that's all.
If you want to reference a type without actual using it, include `schemas/hello_fwd.hpp` with type forward declaration. 
If you want to use some non-standard parser (e.g. for `formats::bson::Value`), include `schemas/hello_parsers.ipp`.

The most common use-case for JSON parser/serializer is a JSON handler:

@snippet samples/chaotic_service/src/hello_service.cpp Handler


### JSONSchema types mapping to C++ types

Base JSONSchema types are mapped to C++ types as following:

| JSONSchema type | C++ type      |
|-----------------|---------------|
| `boolean`       | `bool`        |
| `number`        | `double`      |
| `integer`       | `int`         |
| `string`        | `std::string` |
| `array`         | `std::vector` |
| `object`        | `struct`      |
| `oneOf`         | `std::variant`|
| `allOf`         | `struct`      |
| `$ref`          | -             |


#### type: boolean

Boolean type is mapped to C++ `bool` type.

#### type: integer

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


#### type: number

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


#### type: string

String type is mapped to different C++ types:

| format                | C++ type                     |
|-----------------------|------------------------------|
| -                     | `std::string`                |
| `uuid`                | `boost::uuids::uuid`         |
| `date`                | `utils::datetime::Date`      |
| `date-time`           | `utils::TimePointTz`         |
| `date-time-iso-basic` | `utils::TimePointTzIsoBasic` |

String supports the following validators:
* `minLength`
* `maxLength`
* `pattern`

Please note: `{min,max}Length` relates to UTF-8 code points, not bytes.


#### type: array

Array type is mapped to different C++ types depending on `x-usrv-cpp-container` value:

| x-usrv-cpp-container type | C++ type      |
|---------------------------|---------------|
| -                         | `std::vector` |
| `C`                       | `C`           |

Array supports the following validators:
* `minItems`
* `maxItems`


#### type: object

Object type produces a custom structure C++ type.
Required fields of C++ type `T` produce C++ fields with the same type `T`.
Non-required fields of C++ type `T` produce C++ fields with type `std::optional<T>`.

E.g. the following JSONSchema:
```yaml
type: object
additionalProperties: false
properties:
  foo:
    type: integer
  bar:
    type: string
required:
- foo
```

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
It can be overriden by setting `x-usrv-strict-parsing: false`.
In this case unknown fields will be ignored.


#### oneOf

oneOf type is mapped to C++ `std::variant<...>`.

Parsing function tries to parse input data into all variants of `oneOf` in case of no `mapping`.
It can be very time-consuming in case of huge data types, especially in case of nested `oneOf`s.
So try to use `mapping` everywhere you can to speed up the parsing.


#### allOf

allOf is implemented using multiple inheritance of structures.
It requires that all allOf subcases set `additionalProperties: true`.
Due to implementation details C++ parents' `extra` is not filled during parsing.


#### $ref

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

```yaml
TreeNode:
    type: object
    additionalProperties: false
    properties:
        data:
            type: string
        left:
            $ref: '#/TreeNode'
            x-usrv-cpp-indirect: true
        right:
            $ref: '#/TreeNode'
            x-usrv-cpp-indirect: true
```

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


### User types

One may wrap any generated type using any custom type using `x-usrv-cpp-type` tag.
The tag value is the fully qualified C++ type name you want the value to wrap into.
In case of userver's internal types you may use `userver::` namespace instead of
`USERVER_NAMESPACE`.

Chaotic looks for a header `<userver/chaotic/io/X/Y.hpp>` in all include directories
in case of `x-usrv-cpp-type: X::Y`. The header must contain:

1) the type definition;

2) `Convert` functions (see below). `Convert` function is used to transform user type into JSONSchema type and vice versa.

@include chaotic/integration_tests/include/userver/chaotic/io/my/custom_string.hpp


### Parser

Parsing is implemented in two steps:

1) input data is parsed calling `Parse()` with code-generated parser type as a template parameter

2) the result is wrapped conforming to `x-usrv-cpp-type` tag value

It means that regardless of `x-usrv-cpp-type` value the whole JSONSchema validation magic
is still performed.

The whole parsing process is split into smaller steps using parsers combination.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/formats.md | @ref scripts/docs/en/userver/logging.md ⇨
@htmlonly </div> @endhtmlonly
