# Hierarchical Rule System

This directory contains the production-ready hierarchical rule system for userver C++ development. The system is organized into priority-based layers that provide comprehensive guidance across all development phases and technical domains.

## Directory Structure

```
rules/
├── 00-global/          # Global userver framework rules (Priority: 1000)
├── 10-modes/           # Development mode-specific rules (Priority: 800)
├── 20-themes/          # Technical domain-specific rules (Priority: 600)
├── 30-project/         # Project-specific overrides (Priority: 400)
└── 01-general.md       # Legacy general rules (deprecated)
```

## Rule Hierarchy and Inheritance

### Priority System
- **Higher numbers = Higher priority**
- **00-global**: Framework fundamentals that cannot be overridden
- **10-modes**: Development phase specializations
- **20-themes**: Technical domain expertise
- **30-project**: Project-specific customizations

### Inheritance Flow
1. Global rules are applied first (foundation)
2. Mode-specific rules extend global rules
3. Theme-specific rules add domain expertise
4. Project-specific rules provide final customizations

## Rule Categories

### 00-Global Rules
- **framework-fundamentals.md**: Core userver concepts and patterns
- **component-system.md**: Component lifecycle and architecture
- **async-programming.md**: Coroutine safety and async patterns
- **configuration-management.md**: Static and dynamic configuration
- **error-handling.md**: Exception safety and error patterns
- **performance-guidelines.md**: Performance best practices

### 10-Mode Rules
- **architect/**: System design and architecture planning
- **code/**: Implementation patterns and coding guidelines
- **debug/**: Debugging workflows and troubleshooting
- **ask/**: Learning and documentation patterns
- **orchestrator/**: Multi-phase project coordination

### 20-Theme Rules
- **configuration/**: Configuration management specialization
- **databases/**: Database integration patterns
- **networking/**: Network communication protocols
- **performance/**: Optimization and benchmarking
- **security/**: Security and authentication
- **messaging/**: Messaging systems integration
- **integration/**: External service integration

### 30-Project Rules
- **overrides/**: Project-specific rule modifications
- **customizations/**: Team-specific preferences
- **extensions/**: Project-specific extensions

## Cross-Reference Integration

Each rule file includes:
- **Memory Bank Links**: References to detailed knowledge in memory bank
- **Related Patterns**: Links to implementation patterns
- **Dependencies**: Rule dependencies and prerequisites
- **Examples**: Concrete implementation examples
- **Alternatives**: Alternative approaches for different contexts

## Usage Guidelines

1. **Rule Resolution**: Rules are resolved in priority order with inheritance
2. **Cross-References**: Follow links for detailed implementation guidance
3. **Context Awareness**: Rules adapt based on current development context
4. **Extensibility**: Add new rules using the template system

## Integration with Memory Bank

The rule system seamlessly integrates with the comprehensive memory bank:
- **memory-bank/main/**: Core framework knowledge
- **memory-bank/specialized/**: Advanced patterns and techniques
- **memory-bank/research/**: Experimental features and future directions

## Quality Assurance

All rules follow these quality standards:
- **Actionable**: Provide concrete, implementable guidance
- **Cross-Referenced**: Link to related concepts and patterns
- **Validated**: Tested against real-world userver projects
- **Maintained**: Regularly updated with framework evolution
- **Comprehensive**: Cover all aspects of userver development

## Migration from Legacy Rules

The legacy `01-general.md` file is deprecated. Its content has been:
- **Reorganized**: Distributed across the hierarchical structure
- **Enhanced**: Expanded with memory bank integration
- **Cross-Referenced**: Linked to related patterns and concepts
- **Specialized**: Adapted for different modes and themes

For backward compatibility, the legacy file remains but should not be used for new development.