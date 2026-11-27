## S3 API

**Quality:** @ref QUALITY_TIERS "Platinum Tier".

## Introduction

🐙 **userver** provides client to any endpoint that implements client to any S3
   endpoints. It uses ```namespace s3api```

The client is currently not feature-complete and exposes only some of the possible
methods, parameters and settings. The client also suffers from some
weird design decisions, like explicit selection of "http://" vs "https://". We
intend to gradually improve its quality in consequent releases.

See also:
* @ref scripts/docs/en/userver/tutorial/s3api.md
* @ref s3api::Client

## Usage and testing

* @ref scripts/docs/en/userver/tutorial/s3api.md shows hot to create, use and test client

## Standalone CLI

`userver-s3-cli` executes one S3 operation per invocation using this library's
public client API. It does not run compatibility scenarios, create buckets,
generate object keys, or clean up objects. Users compose their own shell scripts.
The CLI supports virtual-hosted addressing; enable it for the target bucket.

Enabling the S3API library makes the CLI target available. It is excluded from
the default build; build it explicitly with CMake:

```sh
cmake -S . -B build -DUSERVER_FEATURE_S3API=ON
cmake --build build --target userver-s3-cli
```

The executable is `build/libraries/s3api/tools/cli/userver-s3-cli`.

In Arcadia, build from the library directory with:

```sh
ya make tools/cli
```

The executable is `tools/cli/userver-s3-cli`.
Use `--help` to show the accepted arguments. The command and argument names follow
conventional low-level S3 CLI naming. Named credential profiles and additional
commands are not implemented.

Provide a JSON credentials file with nonempty `access_key_id` and `secret_key`
strings. Restrict its permissions, for example with `chmod 600 credentials.json`.
Credentials are not accepted as command-line arguments and are not logged.

| Global option | Meaning | Default |
|---------------|---------|---------|
| `--endpoint-url` | HTTP(S) endpoint without the bucket, URL path, query or fragment | Required |
| `--credentials-file` | Path to the JSON credentials file | Required |
| `--signature-version` | `v2` or `v4` | `v4` |
| `--region` | Signing region; required for V4, rejected for V2 | None |
| `--timeout-ms` | Positive request timeout in milliseconds | `3000` |
| `--retries` | Positive HTTP driver attempt count, including the first attempt | `1` |

The signing region is provider-specific; use the value required by your endpoint.
The examples below use a fictional endpoint and region. V2 and V4 use the existing
library authenticators; no additional signing implementation is involved.

Every command requires `--bucket` and `--key`. Keys are passed unencoded.

```sh
userver-s3-cli --endpoint-url https://s3.example.com \
  --credentials-file credentials.json --region example-region \
  put-object --bucket example-bucket --key probe/head --body head.json \
  --if-none-match '*'

userver-s3-cli --endpoint-url https://s3.example.com \
  --credentials-file credentials.json --region example-region \
  get-object --bucket example-bucket --key probe/head downloaded.json

userver-s3-cli --endpoint-url https://s3.example.com \
  --credentials-file credentials.json --region example-region \
  put-object --bucket example-bucket --key probe/head --body next-head.json \
  --if-match '"etag-from-get"'
```

`get-object` calls `TryGetObject`, requests the `ETag` response header, and requires
a nonempty ETag. It writes the exact object bytes to the positional output file,
overwriting it only after a successful GET. Its stdout is JSON containing
`Success` and `ETag`, for example `{"Success":true,"ETag":"\"value\""}`.

`put-object --body FILE` reads the exact file bytes, including an empty file.
Without a condition it calls `PutObject` with the driver's default content type
`text/plain`. With `--if-match ETAG` or `--if-none-match '*'` it calls
`PutObjectConditional`, whose content type is `application/json`. The conditions
are mutually exclusive. No conditional GET or other `--if-none-match` value is
supported. PUT prints `{"Success":true}`; it does not retrieve an ETag or issue an
additional GET/HEAD request.

Failures are JSON on stderr. HTTP failures preserve the status code, for example
`{"Error":{"Type":"Http","HTTPStatusCode":412,"Message":"S3 request failed"}}`.
Raw response bodies, authorization headers, and credentials are not printed.
The exit codes are `0` for success, `1` for an operation or output-file failure,
and `2` for invalid configuration or an unreadable input file. A failed GET does
not truncate the output file. Local file reads and writes happen outside the
userver task processor; request and response bodies are buffered in memory.

For a conditional-write compatibility check, invoke conditional creation twice
and check that the second command reports HTTP 412. Read the ETag, replace the
object with `--if-match`, and check that another replacement with the previous
ETag reports HTTP 412. Concurrent invocations can test competing writes. Keep
`--retries 1` when observing individual request outcomes. The CLI does not turn
expected errors into successful exits or interpret the results as a provider
compatibility verdict.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/libraries/easy.md | @ref scripts/docs/en/userver/libraries/sqs.md ⇨
@htmlonly </div> @endhtmlonly
