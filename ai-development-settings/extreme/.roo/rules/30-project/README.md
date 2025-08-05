# Project-Specific Overrides

This directory contains project-specific rule overrides and customizations that adapt the global and mode/theme rules to specific project requirements.

## Purpose

Project-specific rules allow teams to:
- Override global rules where project requirements differ
- Add project-specific patterns and conventions
- Customize development workflows for specific contexts
- Maintain team-specific best practices

## Priority and Inheritance

- **Priority**: 400 (lowest in hierarchy)
- **Inherits From**: All higher priority rules (00-global, 10-modes, 20-themes)
- **Override Capability**: Can override rules marked as `override: true`
- **Scope**: Project-specific customizations

## Directory Structure

```
30-project/
├── README.md                    # This file
├── overrides/                   # Rule overrides
│   ├── global-overrides.md      # Overrides for global rules
│   ├── mode-overrides.md        # Overrides for mode-specific rules
│   └── theme-overrides.md       # Overrides for theme-specific rules
├── customizations/              # Project-specific additions
│   ├── team-conventions.md      # Team coding conventions
│   ├── project-patterns.md      # Project-specific patterns
│   └── workflow-customizations.md # Custom development workflows
└── extensions/                  # Project-specific extensions
    ├── custom-components.md     # Custom component patterns
    ├── integration-patterns.md  # Project-specific integrations
    └── testing-extensions.md    # Custom testing approaches
```

## Usage Guidelines

### When to Use Project Overrides
- Project has specific requirements that differ from global best practices
- Team has established conventions that work well for their context
- Integration with legacy systems requires different approaches
- Performance requirements necessitate specific optimizations

### When NOT to Use Project Overrides
- Global rules already provide adequate guidance
- Override would reduce code quality or maintainability
- Change should be contributed back to global rules instead
- Override conflicts with framework fundamentals

## Override Examples

### Safe Override Example
```markdown
# Override for database connection pooling
**Rule ID**: `project.database.connection_pool`
**Overrides**: `theme.databases.postgresql_patterns.connection_pool`
**Justification**: Project requires larger connection pool due to high concurrency

## Custom Connection Pool Configuration
```yaml
components:
    postgres-db:
        min_pool_size: 10        # Override: increased from 4
        max_pool_size: 50        # Override: increased from 15
        max_queue_size: 500      # Override: increased from 200
```
```

### Unsafe Override Example (NOT RECOMMENDED)
```markdown
# ❌ DO NOT DO THIS - Violates framework fundamentals
**Rule ID**: `project.async.blocking_operations`
**Attempts to Override**: `global.async.programming.non_blocking_only`
**Status**: REJECTED - Cannot override framework fundamentals
```

## Template Usage

Use the provided templates to create consistent project overrides:

1. Copy appropriate template from `../../templates/`
2. Replace template variables with project-specific values
3. Ensure override is justified and documented
4. Validate against inheritance rules
5. Test override doesn't break existing functionality

## Validation Rules

Project overrides must pass these validation checks:

### Override Permission Check
- Rule being overridden must have `override: true`
- Cannot override rules with `override: false`
- Must respect inheritance hierarchy

### Quality Standards
- Override must be documented with clear justification
- Must maintain or improve code quality
- Should not conflict with framework fundamentals
- Must include migration path if temporary

### Team Approval
- Significant overrides require team review
- Breaking changes need architecture approval
- Temporary overrides need sunset dates

## Migration Strategy

### Temporary Overrides
For temporary project-specific needs:

```markdown
**Rule ID**: `project.temporary.legacy_integration`
**Override Duration**: 6 months
**Sunset Date**: 2025-07-01
**Migration Plan**: Replace with standard patterns after legacy system upgrade
```

### Permanent Customizations
For long-term project needs:

```markdown
**Rule ID**: `project.permanent.high_performance_config`
**Justification**: Project requires sustained high throughput
**Review Schedule**: Quarterly
**Contribution Potential**: Consider contributing optimizations to global rules
```

## Best Practices

### Documentation Requirements
- Clear justification for each override
- Impact assessment on maintainability
- Migration path for temporary overrides
- Regular review schedule

### Code Quality Maintenance
- Overrides should maintain or improve quality
- Include additional testing for overridden behavior
- Monitor metrics to validate override effectiveness
- Regular review and cleanup of obsolete overrides

### Team Communication
- Document overrides in team knowledge base
- Include in onboarding materials
- Regular team reviews of active overrides
- Clear escalation path for override conflicts

## Integration with CI/CD

### Automated Validation
```yaml
# Example CI validation for project overrides
project_override_validation:
  steps:
    - validate_override_permissions
    - check_rule_inheritance
    - verify_documentation_completeness
    - run_quality_checks
    - validate_test_coverage
```

### Monitoring and Alerts
- Alert on new overrides without proper documentation
- Monitor override usage and effectiveness
- Track override lifecycle and sunset dates
- Report on override impact on code quality metrics

## Examples Directory

See the `examples/` directory for sample project overrides:
- High-performance service configurations
- Legacy system integration patterns
- Team-specific coding conventions
- Custom testing frameworks

---

**Remember**: Project overrides are powerful but should be used judiciously. Always consider whether a change should be contributed to global rules instead of creating a project-specific override.