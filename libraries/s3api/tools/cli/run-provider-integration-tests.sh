#!/usr/bin/bash
# This checks:
# - ordinary PUT
# - binary GET
# - ETag retrieval
# - conditional creation/replacement
# - rejected conditions (412)
# - missing objects (404)

# Set these first:
# 
# export ENDPOINT_URL=https://s3.example.com
# export BUCKET=example-bucket
# export CREDENTIALS_FILE=/path/to/credentials.json
# export SIGNATURE_VERSION=v4
# export REGION=example-region
# export S3_CLI=/path/to/userver-s3-cli

set -euo pipefail

work=$(mktemp -d)
prefix="compatibility/$(basename "$work")"

auth=(--signature-version "${SIGNATURE_VERSION:-v4}")
if [[ "${SIGNATURE_VERSION:-v4}" == v4 ]]; then
    auth+=(--region "${REGION:?Set REGION for V4}")
fi

s3() {
    "$S3_CLI" --endpoint-url "$ENDPOINT_URL" \
        --credentials-file "$CREDENTIALS_FILE" \
        "${auth[@]}" "$@" --bucket "$BUCKET"
}

expect_status() {
    local expected=$1
    shift
    if s3 "$@" > /dev/null 2>"$work/error.json"; then
        echo "Expected HTTP $expected, but operation succeeded" >&2
        exit 1
    fi
    jq -e --argjson expected "$expected" \
        '.Error.HTTPStatusCode == $expected' "$work/error.json" > /dev/null
}

# Ordinary PUT and binary GET.
printf 'binary\000payload\377\n' >"$work/archive"
s3 put-object --key "$prefix/archive" --body "$work/archive"
s3 get-object --key "$prefix/archive" "$work/download"
cmp "$work/archive" "$work/download"

# Conditional creation and duplicate creation.
printf '{"version":1}\n' >"$work/head"
s3 put-object --key "$prefix/head" --body "$work/head" --if-none-match '*'
expect_status 412 put-object --key "$prefix/head" \
    --body "$work/head" --if-none-match '*'

# Retrieve ETag, replace, then reject a stale ETag.
etag=$(s3 get-object --key "$prefix/head" "$work/download" | jq -er '.ETag')
cmp "$work/head" "$work/download"

printf '{"version":2}\n' >"$work/head"
s3 put-object --key "$prefix/head" --body "$work/head" --if-match "$etag"
expect_status 412 put-object --key "$prefix/head" \
    --body "$work/head" --if-match "$etag"

s3 get-object --key "$prefix/head" "$work/download"
cmp "$work/head" "$work/download"

expect_status 404 get-object --key "$prefix/missing" "$work/missing"

echo "PASS; objects remain under $BUCKET/$prefix; local files: $work"
