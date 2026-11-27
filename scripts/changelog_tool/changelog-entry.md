# Writing changelog entries

These writing rules are shared by PR authors and the release changelog tool.
They apply when writing new text. The release tool MUST preserve existing
explicit entries and migration guides verbatim, with the absolute author priority
specified in its prompt; do not rewrite them to apply these writing rules.
Its pass-specific inclusion rules and required JSON fields remain in force,
including writing an acknowledgment for every external leftover.

## Writing a new changelog line

Write in English, usually as a single concise entry describing the main changes
that matter to users of userver: public API changes, performance when the PR is
focused on it, or documentation when that is the focus. PR authors do not need
to classify the change or produce the tool's internal JSON fields.

- For breaking changes, describe ONLY the breaking change itself: WHAT changed.
  Any fix or migration recipe belongs in the separate migration guide, not in the
  changelog line.
- For new functionality, describe the component/subsystem involved and what was
  done (the new capability), in plain user-facing terms.
- For performance changes, focus on NUMBERS whenever the PR author supplies
  measurements (percentages, nanoseconds, bytes, request counts, etc.). State
  WHAT was optimized, WHY, and the MEASURED effect. Numerical performance claims
  MUST use measurements supplied by the PR author. NEVER invent numbers or cite
  synthetic benchmark results. The agent may ask the PR author for measurements.
  Without author-provided measurements, describe the improvement qualitatively,
  without numbers.

## Doxygen formatting

FORMATTING - DOXYGEN-STYLE FOR ALL CODE REFERENCES (applies to "changelog_line" and "migration_guide"):
When these fields mention code entities, files, identifiers, or configuration values, you MUST format them using
Doxygen syntax so the rendered changelog links to the relevant documentation. Apply the following rules strictly:
- userver code entities (classes, functions, methods, namespaces, enumerations, macros, etc.): use the @ref command
 with the fully-qualified name, e.g. @ref userver::components::ComponentBase, @ref userver::server::Server,
 @ref userver::storages::postgres::Cluster, @ref userver::cache::LruCacheComponent.
- Files and directories: use @ref with the path, e.g. @ref userver/core/component.hpp, @ref userver/chaotic/io.hpp.
- Arbitrary inline code fragments, identifiers, variables, parameters, and configuration values: wrap in backticks,
 e.g. `some_variable`, `config.yaml`, `max_concurrent_requests`, `ComponentSystem`.
- Multi-line code blocks: use triple backticks with the language, e.g.
 ```cpp
 auto component = context.FindComponent<components::Foo>();
 ```
- Use @ref for internal cross-references, backticks or @c for inline code, and \link/\endlink only when an extended
  link with custom link text is needed. Markdown links ([text](url)) and HTML tags are also acceptable when they are
  more natural than a Doxygen command (e.g. linking to an external resource). Keep the prose natural and readable;
  only the code references themselves are marked up.

Reference the main changed public API entities in the entry. Follow the
[Doxygen reference syntax](https://www.doxygen.nl/manual/commands.html#cmdref)
and verify the targets when writing new text. Keep release-note text public,
without private links or identifiers.

## Writing a new migration guide

For a breaking change, give a separate, actionable recipe describing HOW users
should adapt their code or configuration. Never merge it with the changelog line
that describes WHAT changed. Where applicable, include minimal before/after code
examples, with language-tagged fenced code blocks and the Doxygen rules above.
Verify examples against the old and new APIs.
