# Writing tests

These rules apply to unit tests (gtest, `UTEST`) and functional tests (testsuite)
when writing tests and reviewing them. For the tooling see
@ref scripts/docs/en/userver/testing.md and
@ref scripts/docs/en/userver/functional_testing.md.

## What to test

Structure the test as setup, act, assert.

New functionality and bug fixes must have tests.

A bug fix needs one test that fails without the fix. For flaky failures caused
by production code rather than the test itself, first add a test that reliably
reproduces the bug without flakiness, then fix the code.

Test behavior, not implementation: assert results observable through the public
API, not internal calls.

If the bug cannot be reproduced through the public API, first look for the public
scenario that triggers it and test that scenario. If there is none, test the
internal component only if it has its own contract and tests;
do not add a test that pins one internal code path.

Keep the test set minimal. Add another test only if it covers a distinct behavior
of the change that no other new or existing test exercises. Tests of unrelated
existing behavior belong to a separate change. When reviewing, request removal
of tests that:
- repeat the same code path with a different API flavor (sync/async, unary/streaming,
  retry/no retry) unless the change affects that flavor specifically;
- check scenarios that are practically unreachable or need artificial setup
  that does not occur in real usage;
- duplicate assertions of existing tests.

## One behavior per test

Each test checks one behavior, and its name states that behavior. A test that
walks through the happy path of several behaviors is fine as long as every failure
gives a clear message. Split a test that checks independent scenarios; a name that
joins several of them signals this. A condition and its expected result describe
one scenario and may both appear in the name.
Add comments only when names and code structure cannot convey the behavior.

Before writing a test, find the closest existing test of the same component
(for example, a failure test next to which the new one belongs) and put the new test
next to it, reusing its fixtures and helpers. Do not construct objects under
test with long positional argument lists; reuse or add a factory helper.

## Test anti-patterns

- Obscure Test: the link between the setup and the expected result is not visible.
- General Fixture: the setup is larger than the scenario needs.
- Conditional Test Logic: branches inside the test change what it checks.
- Erratic Test: the result depends on timing. Wait for events, testpoints or
  mocked time instead of sleeping or relying on timeouts.
- Assertion Roulette: many assertions without context make a failure hard to diagnose.
- Fragile Test: assertions on details outside the contract, such as exact log
  text or unspecified ordering.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/development/code_review.md | @ref scripts/docs/en/userver/driver_guide.md ⇨
@htmlonly </div> @endhtmlonly
