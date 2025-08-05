# Documentation Navigation and Query Guidance

## Overview

Comprehensive guidance for navigating userver documentation, finding specific information, and understanding the documentation structure to efficiently answer developer questions.

## Documentation Structure Understanding

### Primary Documentation Sections

**Core Framework Documentation**
```yaml
main_sections:
  introduction:
    - "I/O-bound Applications and Coroutines"
    - "The Basics" 
    - "Feature Comparison with other Frameworks"
    - "Supported Platforms"
    - "Deploy Environment Specific Configurations"
  
  installation:
    - "Configure, Build and Install"
    - "Build dependencies"
    - "Build options"
    - "Build instructions for samples/tests/benchmarks"
  
  tutorials:
    - "Writing your first HTTP server"
    - "Writing your own configs server"
    - "Production configs and best practices"
    - "TCP/gRPC/Database services"
    - "Authentication/Authorization tutorials"
```

**Specialized Topics**
```yaml
specialized_areas:
  protocols:
    - "gRPC (server/client middlewares)"
    - "HTTP/HTTPS/WebSocket"
    - "RabbitMQ (AMQP 0-9-1)"
    - "Low level: TLS, TCP, UDP sockets"
  
  databases:
    - "PostgreSQL (uPg Driver)"
    - "MySQL/MongoDB/Redis/ClickHouse"
    - "YDB/SQLite"
  
  runtime_features:
    - "Dynamic config"
    - "Logging at runtime"
    - "Service Statistics and Metrics"
    - "Memory profiling"
    - "Deadline propagation"
    - "Congestion Control"
```

## Information Discovery Strategies

### Quick Reference Patterns

**For Component Questions**
```markdown
# When asked about components:
1. Check component system documentation
2. Look in specific component groups:
   - HTTP Handlers: server::handlers::*
   - HTTP Middlewares: server::middlewares::*
   - Clients: clients::*
   - Other components: components::*
```

**For Database Questions**
```markdown
# Database-specific guidance:
PostgreSQL: 
  - Driver: uPg Driver documentation
  - Transactions: "uPg: Transactions"
  - Queries: "uPg: Running queries"
  - Results: "uPg: Working with result sets"
  - Types: "uPg: Supported data types"

MongoDB/Redis/MySQL:
  - Each has dedicated driver documentation
  - Check supported types documentation
  - Review design and implementation details
```

**For Performance Questions**
```markdown
# Performance-related topics:
1. "Profiling context switches"
2. "Memory profiling a production service"
3. "Service Statistics and Metrics"
4. "Congestion Control"
5. "Task Processor Usage Guide"
```

### Documentation Navigation Hierarchy

**Top-Level Navigation**
```yaml
navigation_strategy:
  concept_questions:
    - Start with "Introduction" section
    - Move to "Generic development" for implementation
    - Check "Tutorial" for practical examples
  
  implementation_questions:
    - Begin with relevant tutorial
    - Reference component documentation
    - Check API reference for specific classes
  
  troubleshooting_questions:
    - Start with FAQ section
    - Check "Testing and Benchmarking" for debugging
    - Review specific component troubleshooting guides
```

## Common Query Patterns and Responses

### Framework Basics Queries

**"What is userver?"**
- Direct to main introduction: "The C++ Asynchronous Framework"
- Key points: Modern open source async framework, rich abstractions, efficient I/O
- Emphasize: Solves I/O interactions transparently, avoids CPU-consuming context switches

**"How do I get started?"**
- Point to tutorial progression:
  1. "Writing your first HTTP server"
  2. "Writing your own configs server" 
  3. "Production configs and best practices"
- Mention service templates for quick start

**"What databases are supported?"**
- List all supported databases with links:
  - PostgreSQL (uPg Driver) - most comprehensive
  - MySQL, MongoDB, Redis/Valkey, ClickHouse
  - YDB, SQLite
- Point to specific driver documentation

### Component System Queries

**"How do components work?"**
- Direct to "Component system" documentation
- Explain component lifecycle and registration
- Reference MinimalServerComponentList() usage
- Point to component groups documentation

**"How to create HTTP handlers?"**
- Reference "HTTP handler component" tutorial section
- Key points: Derive from HttpHandlerBase, implement HandleRequest
- Show handler registration in static config
- Point to HTTP Handlers group documentation

### Configuration Queries

**"How to configure services?"**
- Start with static config examples from tutorials
- Reference "Dynamic config" for runtime changes
- Point to "Dynamic config schemas" for available options
- Mention task processors configuration

**"What are task processors?"**
- Reference "Guide on TaskProcessor Usage"
- Explain main-task-processor vs fs-task-processor
- Point to configuration examples in tutorials

### Troubleshooting Queries

**"Service crashes/errors?"**
- Direct to FAQ section first
- Common patterns:
  - Check service logs for hints
  - Analyze core dumps/stacktraces
  - Look for std::terminate in noexcept functions
  - Check utils::Async usage patterns

**"Performance issues?"**
- Reference FAQ performance section
- Point to profiling documentation
- Suggest metrics analysis
- Check for blocking operations in main task processor

**"Database connection issues?"**
- PostgreSQL-specific FAQ sections
- Network timeout vs statement timeout explanation
- Connection pool metrics interpretation
- Point to dynamic config schemas for timeouts

## Cross-Reference Integration

### Memory Bank References
```yaml
memory_bank_integration:
  framework_core: "Core framework concepts and architecture"
  component_system: "Component lifecycle and patterns"
  async_programming: "Asynchronous programming patterns"
  service_patterns: "Service implementation patterns"
  troubleshooting_guide: "Common issues and solutions"
```

### External Documentation Links
```yaml
external_references:
  official_docs: "https://userver.tech/"
  github_repo: "https://github.com/userver-framework/"
  community:
    - "Telegram English: https://t.me/userver_en"
    - "Telegram Russian: https://t.me/userver_ru"
    - "News channel: https://t.me/userver_news"
```

## Response Formatting Guidelines

### Structured Responses
```markdown
# For concept explanations:
1. Brief definition
2. Link to relevant documentation section
3. Key implementation points
4. Related concepts/cross-references
5. Practical examples when available

# For implementation questions:
1. Direct link to tutorial/guide
2. Code example if simple
3. Configuration requirements
4. Common pitfalls to avoid
5. Related documentation links

# For troubleshooting:
1. Reference FAQ first if applicable
2. Diagnostic steps
3. Common causes and solutions
4. Links to debugging tools/techniques
5. When to seek community help
```

### Link Formatting
- Always provide specific documentation section links
- Include both tutorial and reference documentation
- Cross-reference related topics
- Point to community resources when appropriate

## Best Practices for Ask Mode

### Information Prioritization
1. **Start with official documentation** - Most authoritative source
2. **Use tutorial progression** - Builds understanding systematically  
3. **Reference FAQ for common issues** - Saves time on known problems
4. **Cross-reference related topics** - Provides comprehensive understanding
5. **Point to community resources** - For complex or edge cases

### Response Quality Standards
- **Accuracy**: Always verify against official documentation
- **Completeness**: Provide sufficient context and related information
- **Clarity**: Use clear explanations with appropriate technical depth
- **Actionability**: Include specific steps or links for next actions
- **Timeliness**: Reference current documentation versions

### Learning Path Guidance
- **Beginner**: Start with basic tutorials, build understanding progressively
- **Intermediate**: Focus on specific components and patterns
- **Advanced**: Deep dive into performance, debugging, and specialized features
- **Expert**: Architecture decisions, custom components, framework extension