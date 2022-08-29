# Local cache dumps

Cache components derived from components::CachingComponentBase support cache dumps.
If enabled, the cache periodically writes a snapshot of the data to a dump file.
When a cache component starts and a suitable dump exists, the cache reads a dump
and skips the synchronous `Update`.

Advantages:
- Faster service start: the service no longer waits for the first synchronous
  `Update` that may query a database or make some other network requests.
- Better fault tolerance: the service now boots successfully even if the first
  update fails.

Disadvantages:
- CPU time is spent on periodic serialization of data and writing to the dump
  file.
- Disk accesses consume the IO quota, which can negatively affect other disk
  users (for example, logging or other caches).

## How to set up cache dumps
0.  Make a caching component, for example as in @ref md_en_userver_tutorial_http_caching.
1.  Ensure that your data type is dumpable. To do that, add a static_assert
    into the header file of your caching component or near your
    type declaration:
    \snippet core/src/dump/class_serialization_sample_test.hpp  Sample class serialization dump header
2.  If the assertion fails:
    - @ref dump_serialization_guide "Implement serialization"
    - @ref dump_testing_guide "Write unit tests for it"
3.  Enable dumps in the static configuration of your service:
  ```
  yaml
  components_manager:
    components:
      simple-dumped-cache:
        update-types: only-full
        update-interval: 1m
        # that's how we turn it on
        dump:
          enable: true
          world-readable: false
          format-version: 0
          first-update-mode: required
          first-update-type: full
  ```

@anchor dump_serialization_guide
## Implementing serialization (Write / Read)

In order for a data type to be serialized for cache dumps the `Write` and
`Read` functions must be implemented in the namespace of the type or in
`namespace dump`:

- Basic types already have the required functions defined in
@ref userver_dump_read_write "various headers":
    * Integers, floats, doubles, strings, `std::chrono`, `enum`, `uuid` in
      `<userver/dump/common.hpp>`
    * C++ Standard Library and Boost containers, `std::optional`,
      `utils::StrongTypedef`, `std::{unique,shared}_ptr` in
      `<userver/dump/common_containers.hpp>`
    * Trivial structures (aggregates) in `<userver/dump/aggregates.hpp>`, but
      you will have to enable dumps manually:
  \snippet core/src/dump/aggregates_sample_test.cpp Sample aggregate dump
- For more complex user types it is recommended to place only declarations in
  the header file to avoid unnecessary includes leaking into all the translation
  units that use the type:
  \snippet core/src/dump/class_serialization_sample_test.hpp  Sample class serialization dump header
  Actual definitions should go to the source file:
  \snippet core/src/dump/class_serialization_sample_test.cpp  Sample class serialization dump source
- If JSON, protobuf or flatbuffers serialization is already implemented for the
  structure, you can use it in `Write`/`Read` if it suites your requirements.

### Recommendations

1. Place `Write`/`Read` function declarations for your own types:
  - in the `namespace` of the type, not into `namespace dump`
  - in the same header, next to the type definition
2. Place serialization of third party types:
  - in `namespace dump`
  - if serialization may be useful to others and does not add source
    dependencies, it is worth making a PR into userver
3. When implementing serialization for multiple types, place functions in the
   order `Write1, Read1, Write2, Read2, ...`
4. Avoid using `Write/Read` functions by ADL (argument dependent lookup),
   prefer calling `writer.Write(value)` or `reader.Read<T>()`.


@anchor dump_testing_guide
## Testing serialization

If you have written your own `Write/Read` functions, write `utest` tests for
them using `<userver/dump/test_helpers.hpp>`. If the data type supports
`operator==`, it is sufficient to use dump::TestWriteReadCycle.


## Encryption of the dump file

By default, the data in the file is stored using an insecure format
(binary or JSON). A user with access to the dump file can restore its contents.
If the dump file is compromised, a malicious user can gain access to the data.
This may be undesirable in the case of caches that store personal or other
private data. For such caches, you can enable encryption of the dump
file. Dump encryption is disabled by default.

The components::CachingComponentBase uses the AES-GCM-256 encryption algorithm
if the encryption is enabled. To enable encryption, do the following:
1.  In the static configuration for the cache, set `dump.encrypted=true`:
   ```
   yaml
   components_manager:
     components:
       your-caching-component:
         dump:
           encrypted: true
   ```
2. Place a secret for encoding and decoding into `CACHE_DUMP_SECRET_KEYS`
    section of the components::Secdist file. Use the name of your cache
    component as a JSON key and the secret as a JSON value:
    ```
    json
    {
      "CACHE_DUMP_SECRET_KEYS": {
          "you-cache-component-name": "your-base64encoded-secret-key"
      }
    }
    ```

## Dump Settings

Static settings for dumps are set in the `dump` subsection of the cache
component. All the `dump` options are described in the dump::Dumper.

An example with all the options:

```
yaml
components_manager:
  components:
  simple-dumped-cache:
    update-types: only-incremental
    update-interval: 1m
    dump:
      enable: true
      world-readable: false
      format-version: 4
      first-update-mode: skip
      first-update-type: incremental
      max-age: 60m
      max-count: 1
      min-interval: 3m
      fs-task-processor: my-task-processor
      wait-for-first-update: true
      encrypted: false
```

## Dynamic configuration of dumps

A subset of dump settings could be overridden by the dynamic configuration
@ref USERVER_DUMPS.

If you omit setting the dynamic configuration (or do not describe some cache
in @ref USERVER_DUMPS), the value from the static configuration is used. To
reset to the default value, you need to specify `null` in the dynamic
configuration.

Example:

```json
{
  "your-cache-component": {
    "dumps-enabled": true,
    "min-dump-interval-ms": 600000
  }
}
```


## Implementation details
### IO 

- Disk operations are performed asynchronously with `Update` in a separate
 `fs-task-processor`. `Write` and `Read` are also called in that task processor.
- No more than one dump could be written at the same time. If after an
  `Update` the previous write to the dump has not completed, the new write
  operation is skipped.
- While writing the dump dump::FileWriter periodically calls to
  engine::Yield to avoid blocking the thread for a long time
- For each cache, a subdirectory with the name of the cache is created
- The dump name contains UTC time with microsecond precision and
  `format-version`, for example `2020-10-28T174608.907090Z-v0`
- Dump is updated atomically. A new `.tmp` file is created, flushed on the disk
  and atomically renamed.
- Permissions for all the created directories are `0755`
- Permissions for the dump files are either `0400` or `0444`, depending on the
  `world-readable` setting.


### Data format

- The dump data format is a binary stream of fixed-size structures and strings
  in the "size, data" format.
- Sizes and other integers are compressed. `0` and other small numbers occupy
  1 byte. Large and negative numbers take up to 9 bytes
- Empty `std::string`, `std::optional`, containers occupy 1 byte
- Optimization of default values is not performed
- Format of the dump is platform-independent


## Nuances and pitfalls

- If an empty value is stored in the cache (for example, due to calling
  CachingComponentBase::Clear()), the cache will gladly replace the old dump
  with a new empty dump. You can deal with this in the following ways:
    - Do not use Clear(). The cache expects that if there is no data, it is
      `nullptr` that lies there. To prevent this from causing an exception, you
      will have to redefine `void MayReturnNull() override { return true; }`
      and `first-update-fail-ok: true`
- Redefine `Write`/`Read` for your data type to throw `cache::EmptyCacheError`
  when the cache value is empty. If there is a standard datatype in the cache,
  you will have to wrap it in a `struct` to define a custom `Write`/`Read`.
- If multiple `std::shared_ptr`s for the same object are stored in the same cache
  snapshot, they will point to separate copies of this object after the dump
  is loaded.
- In order not to copy a large string when deserializing JSON, flatbuffers,
  etc., you can use `<userver/dump/unsafe.hpp>`


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_caches | @ref pg_cache ⇨
@htmlonly </div> @endhtmlonly

