# Issuing and verifying JSON Web Tokens

@ref crypto::jwt provides functions to issue, parse, and verify signed JSON Web
Tokens (JWTs). Include `<userver/crypto/jwt.hpp>` and link `userver::universal`.

## Issue a token

Pass a JSON object containing the claims and a @ref crypto::Signer from
`<userver/crypto/signers.hpp>` to @ref crypto::jwt::MakeToken. The signer
determines the `alg` header and signs the compact token with `typ` set to `JWT`.
When constructing claims in code, use unique member names in each JSON object,
including nested objects. The default verification policy requires an `exp` claim with the
expiration time in seconds since the Unix epoch. This token carries a user ID
and expires in 15 minutes.

@snippet universal/src/crypto/jwt_test.cpp jwt issue

If you already have serialized JSON, pass it as a `std::string_view` to
@ref crypto::jwt::MakeToken. These overloads preserve the payload bytes,
including whitespace, and leave registered-claim validation to the caller.
They write `typ` first and `alg` second. A third argument adds `kid` after `alg`,
even when the key ID is an empty string.
Passing @ref crypto::SignerNone produces an unsigned token, which @ref crypto::jwt::Verify rejects.

## Verify an authorization token

Include `<userver/crypto/verifiers.hpp>` and create a @ref crypto::Verifier
with the trusted issuer's key and the algorithm your application expects.
For example, use @ref crypto::VerifierRs256 with a
PEM public key or @ref crypto::VerifierHs256 with a shared secret. Keep the
verifier for reuse across requests. Use RSA keys of at least 2048 bits.

Pass the token itself, without the `Bearer ` prefix, to @ref crypto::jwt::Verify.
Specify the expected issuer and audience to prevent accepting tokens intended
for another issuer or service. The following example is exercised by unit tests;
`token` is the compact token and `verifier` holds its trusted verification key:

@snippet universal/src/crypto/jwt_test.cpp jwt verify

The function returns @ref crypto::jwt::Token only after signature and claim
validation succeeds. It throws @ref crypto::VerificationError on invalid input.
Exceptions do not include token contents. Authorization remains application-specific:
check roles or scopes, the subject's access to the requested resource, and any
revocation policy before granting access.

For HTTP handlers, call the verifier from an implementation of
@ref server::handlers::auth::AuthCheckerBase and convert verification failures to
an authentication failure. See
@ref scripts/docs/en/userver/tutorial/auth_postgres.md for auth checker
registration and passing user data to handlers.

## Validation options

@ref crypto::jwt::VerificationOptions controls claim validation:

| Field | Behavior |
| ----- | -------- |
| `issuer` | Require an exact match for `iss`. |
| `audiences` | With an empty set, accept only tokens without `aud`. With a nonempty set, require an `aud` string or array with at least one matching recipient. |
| `subject` | Require an exact match for `sub`. |
| `type` | Compare the `typ` media type name case-insensitively, ignoring parameters. For example, `at+jwt` matches `application/AT+JWT; profile=v1`. Use the full media type when a parameter contains `/`. |
| `leeway` | Nonnegative clock skew allowance, in seconds; defaults to zero. |
| `expiration` | Require `exp` by default; @ref crypto::jwt::ExpirationPolicy::kOptional allows its absence. |

Issuing from a @ref formats::json::Value and verifying with @ref crypto::jwt::Verify
both check the types of registered claims. The `iss` and
`sub` claims and each audience accept any JSON string, including strings with
colons. Verification compares these values exactly. Expiration is always checked
when present, even with optional expiration. The current time must be strictly before
`exp + leeway`, at least `nbf - leeway`, and at least `iat - leeway`. Missing
`nbf` and `iat` are allowed. NumericDates may contain fractional seconds.
Time comes from @ref utils::datetime::Now and can be mocked in tests with
@ref utils::datetime::MockNowSet.

## Migrating from cpp-jwt

The [realmedium_sample](https://github.com/userver-framework/realmedium_sample/blob/9920fb071a896592828e93f17e5e7be91f659a4f/src/utils/jwt.cpp#L7-L14)
issues HS256 tokens containing a string `id` and omits `exp`. To keep that format,
pass the same claims to @ref crypto::jwt::MakeToken and set
@ref crypto::jwt::ExpirationPolicy::kOptional when calling @ref crypto::jwt::Verify.
The sample uses a six-byte key. Before migrating, configure its issuer and
verifiers with a new shared key of at least 32 cryptographically random bytes.
Reissue existing tokens with the new key. Check the configured key length
when constructing the application's signer and verifier.

## Parsing and key selection

When the issuer uses multiple keys, call @ref crypto::jwt::UnverifiedParseJoseHeader
on the compact token. It reads the header without decoding the payload or
checking the signature. Use the header's `key_id` to select a verifier from
the application's trusted keys. Pass the original token, the selected verifier,
and the application's @ref crypto::jwt::VerificationOptions to @ref crypto::jwt::Verify.
It checks the signature before parsing the payload. Use the verified claims for authorization.

@snippet universal/src/crypto/jwt_test.cpp jwt key selection

The library does not fetch keys, follow `jku` or `x5u` URLs, or trust keys embedded
in a token. Key distribution, rotation, and binding keys to issuers are the
application's responsibility.

## Supported token formats

@ref crypto::jwt::Verify supports HS256/384/512, RS256/384/512, PS256/384/512, and
ES256/384/512, using the existing @ref crypto::Verifier implementations.
The `alg` header must be a nonempty ASCII string and match the supplied verifier
exactly. Verification rejects unsigned tokens (`alg: none`), encrypted or nested JWTs,
critical header extensions (`crit`), and the unencoded payload extension (`b64`).
A `cty` of `JWT` or `application/jwt` identifies a nested JWT even when it includes parameters.

@ref crypto::jwt::Verify requires three nonempty, canonical, unpadded base64url segments.
The header and payload must be UTF-8 JSON objects with unique member names. The verified
@ref crypto::jwt::Token contains a @ref crypto::jwt::JoseHeader with
`algorithm`, `type`, `content_type`, and `key_id` for the JOSE parameters
`alg`, `typ`, `cty`, and `kid`. Its `extensions` map preserves other noncritical
parameters and their JSON values. Use the usual userver JSON conversions to
read or write the header. Serialization omits absent optional fields.
The returned header owns its data. The verified token owns its header and payload
and retains all custom claims.

For the JWT and JWS specifications, see [RFC 7519](https://www.rfc-editor.org/rfc/rfc7519)
and [RFC 7515](https://www.rfc-editor.org/rfc/rfc7515). For guidance on binding
algorithms to keys, see [RFC 8725](https://www.rfc-editor.org/rfc/rfc8725).

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/periodics.md | @ref scripts/docs/en/userver/codegen_overview.md ⇨
@htmlonly </div> @endhtmlonly
