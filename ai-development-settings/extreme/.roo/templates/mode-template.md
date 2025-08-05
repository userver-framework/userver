# {{MODE_NAME}} Mode Rules Template

**Rule ID**: `mode.{{mode_slug}}.{{rule_name}}`  
**Priority**: 800  
**Scope**: {{mode_slug}}-mode  
**Override**: true  
**Inherits**: [`global.framework.fundamentals`](../../00-global/framework-fundamentals.md){{#additional_dependencies}}, [`{{.}}`]({{dependency_path}}){{/additional_dependencies}}

## {{MODE_NAME}} Mode Overview

### Mode Purpose
{{mode_description}}

### Key Responsibilities
{{#responsibilities}}
- {{.}}
{{/responsibilities}}

### Mode-Specific Patterns
{{mode_specific_patterns}}

**Memory Bank Reference**: [`memory-bank://{{memory_bank_category}}/{{memory_bank_entry}}`](../../memory-bank/{{memory_bank_category}}/{{memory_bank_entry}}.md)

## Core {{MODE_NAME}} Patterns

### Primary Pattern Category
{{primary_pattern_description}}

```cpp
// Example implementation for {{MODE_NAME}} mode
namespace {{namespace}}::{{mode_slug}} {

class {{ExampleClassName}} {
private:
    {{#private_members}}
    {{type}} {{name}};
    {{/private_members}}
    
public:
    {{ExampleClassName}}({{#constructor_params}}{{type}} {{name}}{{#not_last}}, {{/not_last}}{{/constructor_params}});
    
    {{#public_methods}}
    {{return_type}} {{method_name}}({{#parameters}}{{type}} {{name}}{{#not_last}}, {{/not_last}}{{/parameters}}) {{async_qualifier}};
    {{/public_methods}}

private:
    {{#private_methods}}
    {{return_type}} {{method_name}}({{#parameters}}{{type}} {{name}}{{#not_last}}, {{/not_last}}{{/parameters}}) {{async_qualifier}};
    {{/private_methods}}
};

} // namespace {{namespace}}::{{mode_slug}}
```

### Implementation Guidelines
{{#implementation_guidelines}}
- **{{guideline_category}}**: {{guideline_description}}
{{/implementation_guidelines}}

**Cross-Reference**: [`pattern://{{pattern_domain}}/{{pattern_name}}`](../../memory-bank/{{pattern_category}}/{{pattern_file}}.md#{{pattern_anchor}})

## {{MODE_NAME}}-Specific Workflows

### Workflow 1: {{workflow_1_name}}
{{workflow_1_description}}

```cpp
// Workflow implementation example
{{workflow_1_code_example}}
```

### Workflow 2: {{workflow_2_name}}
{{workflow_2_description}}

```yaml
# Configuration example for {{workflow_2_name}}
{{workflow_2_config_example}}
```

## Integration Patterns

### Integration with Other Modes
{{#mode_integrations}}
- **{{mode_name}}**: {{integration_description}}
{{/mode_integrations}}

### Integration with Themes
{{#theme_integrations}}
- **{{theme_name}}**: {{integration_description}}
{{/theme_integrations}}

**Memory Bank Reference**: [`memory-bank://{{integration_category}}/{{integration_entry}}`](../../memory-bank/{{integration_category}}/{{integration_entry}}.md)

## Quality Standards for {{MODE_NAME}} Mode

### Code Quality Requirements
{{#quality_requirements}}
- **{{requirement_category}}**: {{requirement_description}}
{{/quality_requirements}}

### Testing Requirements
{{#testing_requirements}}
- **{{test_type}}**: {{test_description}}
{{/testing_requirements}}

### Documentation Requirements
{{#documentation_requirements}}
- **{{doc_type}}**: {{doc_description}}
{{/documentation_requirements}}

## Performance Considerations

### {{MODE_NAME}} Performance Patterns
{{performance_patterns_description}}

```cpp
// Performance optimization example
{{performance_code_example}}
```

### Resource Management
{{resource_management_description}}

**Cross-Reference**: [`concept://performance/{{performance_concept}}`](../../memory-bank/research/performance-research.md#{{performance_anchor}})

## Error Handling in {{MODE_NAME}} Mode

### Mode-Specific Error Patterns
{{error_handling_description}}

```cpp
// Error handling example
namespace {{namespace}}::{{mode_slug}}::errors {

class {{MODE_NAME}}Error : public ServiceError {
public:
    explicit {{MODE_NAME}}Error(const std::string& message)
        : ServiceError(message, "{{ERROR_CODE_PREFIX}}_ERROR") {
    }
};

{{#specific_errors}}
class {{error_class}} : public {{MODE_NAME}}Error {
public:
    explicit {{error_class}}(const std::string& message)
        : {{MODE_NAME}}Error(message) {
    }
};
{{/specific_errors}}

} // namespace {{namespace}}::{{mode_slug}}::errors
```

## Cross-References

### Related Memory Bank Entries
{{#memory_bank_references}}
- [`memory-bank://{{category}}/{{entry}}`](../../memory-bank/{{category}}/{{entry}}.md) - {{description}}
{{/memory_bank_references}}

### Implementation Examples
{{#implementation_examples}}
- [`example://{{example_type}}/{{example_name}}`]({{example_path}}) - {{example_description}}
{{/implementation_examples}}

### Alternative Approaches
{{#alternatives}}
- [`alternative://{{alternative_name}}`]({{alternative_path}}) - {{alternative_description}}
{{/alternatives}}

---

**Mode Context**: {{MODE_NAME}} mode {{mode_context_description}}.  
**Inheritance**: Extends global rules with {{mode_slug}}-specific guidance.  
**Dependencies**: {{#dependencies}}[`{{.}}`]({{dependency_path}}){{#not_last}}, {{/not_last}}{{/dependencies}}  
**Last Updated**: {{current_date}}  
**Next Review**: {{review_date}}

---

## Template Variables Reference

This template uses the following variables that should be replaced when creating a new mode:

### Required Variables
- `{{MODE_NAME}}`: Display name of the mode (e.g., "Architect", "Code", "Debug")
- `{{mode_slug}}`: URL-safe slug for the mode (e.g., "architect", "code", "debug")
- `{{rule_name}}`: Specific rule name within the mode
- `{{mode_description}}`: Brief description of the mode's purpose
- `{{namespace}}`: C++ namespace for the mode-specific code

### Optional Variables
- `{{memory_bank_category}}`: Category in memory bank (main, specialized, research)
- `{{memory_bank_entry}}`: Specific memory bank entry name
- `{{pattern_domain}}`: Domain for cross-reference patterns
- `{{ExampleClassName}}`: Example class name for code samples
- `{{current_date}}`: Current date in YYYY-MM-DD format
- `{{review_date}}`: Next review date

### Array Variables
- `{{#responsibilities}}`: List of mode responsibilities
- `{{#implementation_guidelines}}`: List of implementation guidelines
- `{{#quality_requirements}}`: List of quality requirements
- `{{#memory_bank_references}}`: List of related memory bank entries

### Usage Instructions
1. Copy this template to create a new mode rule file
2. Replace all `{{variable}}` placeholders with appropriate values
3. Remove unused sections and array items
4. Add mode-specific content and examples
5. Update cross-references to point to actual files
6. Validate the rule follows the inheritance hierarchy