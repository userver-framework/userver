# Cross-Reference Template

This template defines the structure for creating comprehensive cross-references between rules, memory bank entries, and implementation patterns.

## Cross-Reference Types

### Memory Bank Links
Links to detailed knowledge in the memory bank system:

```markdown
**Memory Bank Reference**: [`memory-bank://{{category}}/{{entry}}`]({{relative_path}}) - {{description}}
```

**Template Variables:**
- `{{category}}`: main, specialized, research
- `{{entry}}`: specific entry name
- `{{relative_path}}`: relative path to the memory bank file
- `{{description}}`: brief description of the linked content

### Pattern References
Links to implementation patterns:

```markdown
**Cross-Reference**: [`pattern://{{domain}}/{{pattern_name}}`]({{pattern_path}}) - {{pattern_description}}
```

**Template Variables:**
- `{{domain}}`: technical domain (database, networking, performance, etc.)
- `{{pattern_name}}`: specific pattern identifier
- `{{pattern_path}}`: path to pattern documentation
- `{{pattern_description}}`: brief pattern description

### Rule Dependencies
Links to other rules in the hierarchy:

```markdown
**Inherits**: [`{{rule_id}}`]({{rule_path}})
**Depends On**: [`{{dependency_rule_id}}`]({{dependency_path}})
**Extends**: [`{{parent_rule_id}}`]({{parent_path}})
```

**Template Variables:**
- `{{rule_id}}`: unique rule identifier
- `{{rule_path}}`: relative path to the rule file
- `{{dependency_rule_id}}`: ID of dependent rule
- `{{dependency_path}}`: path to dependency rule

### Conceptual Links
Links to related concepts and ideas:

```markdown
**Cross-Reference**: [`concept://{{domain}}/{{concept_name}}`]({{concept_path}}) - {{concept_description}}
```

**Template Variables:**
- `{{domain}}`: conceptual domain
- `{{concept_name}}`: specific concept identifier
- `{{concept_path}}`: path to concept documentation
- `{{concept_description}}`: brief concept description

### Implementation Examples
Links to concrete implementation examples:

```markdown
**Implementation Example**: [`example://{{type}}/{{example_name}}`]({{example_path}}) - {{example_description}}
```

**Template Variables:**
- `{{type}}`: example type (basic, advanced, production, etc.)
- `{{example_name}}`: specific example identifier
- `{{example_path}}`: path to example code/documentation
- `{{example_description}}`: brief example description

### Alternative Approaches
Links to alternative implementation approaches:

```markdown
**Alternative Approach**: [`alternative://{{approach_name}}`]({{approach_path}}) - {{approach_description}}
```

**Template Variables:**
- `{{approach_name}}`: alternative approach identifier
- `{{approach_path}}`: path to alternative documentation
- `{{approach_description}}`: brief description of the alternative

## Cross-Reference Sections Template

### Standard Cross-Reference Section
Every rule should include this section at the end:

```markdown
## Cross-References

### Related Memory Bank Entries
{{#memory_bank_entries}}
- [`memory-bank://{{category}}/{{entry}}`]({{path}}) - {{description}}
{{/memory_bank_entries}}

### Implementation Examples
{{#examples}}
- [`example://{{type}}/{{name}}`]({{path}}) - {{description}}
{{/examples}}

### Alternative Approaches
{{#alternatives}}
- [`alternative://{{name}}`]({{path}}) - {{description}}
{{/alternatives}}

### Related Patterns
{{#patterns}}
- [`pattern://{{domain}}/{{name}}`]({{path}}) - {{description}}
{{/patterns}}

### Conceptual Links
{{#concepts}}
- [`concept://{{domain}}/{{name}}`]({{path}}) - {{description}}
{{/concepts}}
```

## Bidirectional Cross-Reference Template

### Forward References
References from current rule to related content:

```markdown
### Forward References
- **Implements**: [`pattern://{{domain}}/{{pattern}}`]({{path}})
- **Uses**: [`memory-bank://{{category}}/{{entry}}`]({{path}})
- **Extends**: [`rule://{{hierarchy}}/{{rule}}`]({{path}})
```

### Backward References
References from related content back to current rule:

```markdown
### Backward References
- **Used By**: [`rule://{{hierarchy}}/{{rule}}`]({{path}})
- **Extended By**: [`rule://{{hierarchy}}/{{rule}}`]({{path}})
- **Referenced In**: [`memory-bank://{{category}}/{{entry}}`]({{path}})
```

## Context-Aware Cross-References

### Mode-Specific References
Cross-references that adapt based on current mode:

```markdown
{{#if architect_mode}}
**Architecture Reference**: [`memory-bank://main/service-patterns#architecture`]({{path}})
{{/if}}

{{#if code_mode}}
**Implementation Reference**: [`memory-bank://main/service-patterns#implementation`]({{path}})
{{/if}}

{{#if debug_mode}}
**Debugging Reference**: [`memory-bank://main/troubleshooting-guide#debugging`]({{path}})
{{/if}}
```

### Theme-Specific References
Cross-references that adapt based on active themes:

```markdown
{{#if database_theme}}
**Database Pattern**: [`pattern://database/{{pattern_name}}`]({{path}})
{{/if}}

{{#if networking_theme}}
**Network Pattern**: [`pattern://networking/{{pattern_name}}`]({{path}})
{{/if}}

{{#if performance_theme}}
**Performance Pattern**: [`pattern://performance/{{pattern_name}}`]({{path}})
{{/if}}
```

## Cross-Reference Validation Template

### Link Validation Rules
```yaml
validation_rules:
  memory_bank_links:
    - check_file_exists: true
    - validate_anchor: true
    - check_category_match: true
  
  pattern_links:
    - check_pattern_registry: true
    - validate_domain: true
    - check_implementation_exists: true
  
  rule_links:
    - check_rule_hierarchy: true
    - validate_inheritance: true
    - check_circular_dependencies: false
```

### Broken Link Handling
```markdown
<!-- Broken link placeholder -->
**[BROKEN LINK]**: `{{original_link}}` - {{error_message}}
**Suggested Alternative**: [`{{alternative_link}}`]({{alternative_path}})
```

## Usage Instructions

### Creating New Cross-References
1. Identify the type of cross-reference needed
2. Use the appropriate template from above
3. Replace all `{{variable}}` placeholders
4. Validate links exist and are accessible
5. Test bidirectional relationships

### Maintaining Cross-References
1. Regular validation of link integrity
2. Update references when files move
3. Add new references when content is created
4. Remove obsolete references during cleanup

### Best Practices
1. **Specificity**: Link to specific sections, not just files
2. **Context**: Provide meaningful descriptions for all links
3. **Bidirectionality**: Ensure important relationships work both ways
4. **Validation**: Regularly check link integrity
5. **Relevance**: Only include truly relevant cross-references

## Integration with Memory Bank

### Memory Bank Cross-Reference Format
```markdown
**Rule Integration**: This memory bank entry is referenced by:
{{#referencing_rules}}
- [`{{rule_id}}`]({{rule_path}}) - {{rule_description}}
{{/referencing_rules}}

**Pattern Usage**: This entry provides patterns used in:
{{#pattern_usage}}
- [`{{usage_context}}`]({{usage_path}}) - {{usage_description}}
{{/pattern_usage}}
```

### Automatic Cross-Reference Generation
```yaml
auto_generation:
  enabled: true
  confidence_threshold: 0.8
  manual_review_required: true
  
  sources:
    - memory_bank_content_analysis
    - rule_content_analysis
    - code_example_analysis
    - pattern_relationship_analysis
```

This template system ensures comprehensive and maintainable cross-references throughout the rule system.