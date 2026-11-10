# JWT verification

@ref crypto::jwt provides parsing and verification of signed JSON Web Tokens
in `userver::universal`. Include `<userver/crypto/jwt.hpp>` and
`<userver/crypto/verifiers.hpp>`, and link `userver::universal`.

## Verify an authorization token

Create a @ref crypto::Verifier with the trusted issuer's key and the algorithm
your application expects. For example, use @ref crypto::VerifierRs256 with a
PEM public key or @ref crypto::VerifierHs256 with a shared secret. Keep the
verifier for reuse across requests. HMAC secrets must have sufficient random
entropy and be at least as long as the hash output (32/48/64 bytes for
HS256/384/512); use RSA keys of at least 2048 bits.

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
| `audience` | Require a matching `aud` string or an element of its string array. |
| `subject` | Require an exact match for `sub`. |
| `type` | Require an exact match for the `typ` header, for example `JWT` or `at+jwt`. |
| `leeway` | Nonnegative clock skew allowance, in seconds; defaults to zero. |
| `expiration` | Require `exp` by default; @ref crypto::jwt::ExpirationPolicy::kOptional allows its absence. |

Unset expected values do not constrain the corresponding claims, but registered
claims are still checked for correct types. Expiration is always checked when
present, even with optional expiration. The current time must be strictly before
`exp + leeway`, at least `nbf - leeway`, and at least `iat - leeway`. Missing
`nbf` and `iat` are allowed. NumericDates may contain fractional seconds.
Time comes from @ref utils::datetime::Now and can be mocked in tests with
@ref utils::datetime::MockNowSet.

## Parsing and key selection

@ref crypto::jwt::Parse decodes the header and payload without validating the
signature or claims. Treat every value it returns as untrusted. It can be used
to look up `kid` in an application-maintained collection of trusted keys, followed
by @ref crypto::jwt::Verify with the selected verifier and the original token.
Do not use parsed claims for authorization before verification.

The library does not fetch keys, follow `jku` or `x5u` URLs, or trust keys embedded
in a token. Key distribution, rotation, and binding keys to issuers are the
application's responsibility.

## Supported token formats

Supported algorithms are HS256/384/512, RS256/384/512, PS256/384/512, and
ES256/384/512, using the existing @ref crypto::Verifier implementations.
The header's `alg` must match the supplied verifier exactly. Unsigned tokens
(`alg: none`), encrypted or nested JWTs, critical header extensions (`crit`),
and the unencoded payload extension (`b64`) are unsupported.

Tokens must use three nonempty, canonical, unpadded base64url segments. Header
and payload must be UTF-8 JSON objects with unique member names. Unknown,
noncritical header fields and custom claims are preserved for the application.
The returned JSON values own their data independently of the input token.

The format and validation rules follow [RFC 7519](https://www.rfc-editor.org/rfc/rfc7519),
[RFC 7515](https://www.rfc-editor.org/rfc/rfc7515), and the algorithm binding
guidance in [RFC 8725](https://www.rfc-editor.org/rfc/rfc8725).
