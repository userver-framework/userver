# Orchestrator Mode: Team Collaboration Rules

## Overview
This file defines orchestration patterns for coordinating team collaboration in complex userver development environments, focusing on multi-team coordination, communication protocols, and collaborative development workflows.

## Multi-Team Coordination Patterns

### Team Structure Orchestration
- **Service Ownership Models**: Define clear team responsibilities
  - Assign service ownership based on domain expertise
  - Coordinate shared component ownership across teams
  - Plan cross-team collaboration for shared services
  - Design team boundary management strategies

- **Cross-Team Dependencies**: Manage inter-team dependencies
  - Map team dependency relationships
  - Coordinate shared library development
  - Plan API contract collaboration
  - Design dependency resolution workflows

### Communication Protocol Design
- **Structured Communication**: Establish communication frameworks
  - Design technical decision communication protocols
  - Plan architecture review processes
  - Coordinate design document workflows
  - Establish escalation procedures

- **Knowledge Sharing Orchestration**: Coordinate knowledge transfer
  - Plan technical knowledge sharing sessions
  - Coordinate documentation responsibilities
  - Design mentoring and onboarding workflows
  - Plan expertise distribution strategies

## Development Workflow Coordination

### Code Collaboration Patterns
- **Trunk-Based Development**: Coordinate trunk-based workflows
  - Plan feature branch strategies across teams
  - Coordinate merge conflict resolution
  - Design code review distribution
  - Plan continuous integration coordination

- **Code Review Orchestration**: Coordinate review processes
  - Design cross-team code review workflows
  - Plan expertise-based review assignment
  - Coordinate review quality standards
  - Design review feedback integration

### Version Control Coordination
- **Multi-Repository Management**: Coordinate across repositories
  - Plan repository access and permissions
  - Coordinate branching strategies
  - Design release branch coordination
  - Plan repository synchronization

- **Shared Component Development**: Coordinate shared code
  - Plan shared library development workflows
  - Coordinate component API evolution
  - Design backward compatibility strategies
  - Plan component testing coordination

## Technical Decision Coordination

### Architecture Decision Management
- **Collaborative Architecture**: Coordinate architectural decisions
  - Design architecture review boards
  - Plan technical RFC processes
  - Coordinate design document reviews
  - Design consensus building workflows

- **Technology Stack Coordination**: Manage technology choices
  - Coordinate [`userver`](../../memory-bank/main/framework-core.md:1) version management
  - Plan technology evaluation processes
  - Design technology adoption workflows
  - Coordinate technology migration strategies

### Standards and Guidelines Coordination
- **Coding Standards**: Coordinate development standards
  - Plan coding style guide development
  - Coordinate linting and formatting rules
  - Design code quality metrics
  - Plan standards enforcement workflows

- **API Design Coordination**: Coordinate API development
  - Plan API design review processes
  - Coordinate API versioning strategies
  - Design API documentation standards
  - Plan API testing coordination

## Development Environment Coordination

### Shared Development Infrastructure
- **Development Environment Standardization**: Coordinate dev environments
  - Plan [`Dev Container`](../../memory-bank/main/framework-core.md:1) standardization
  - Coordinate development tool selection
  - Design environment provisioning workflows
  - Plan environment troubleshooting support

- **Build System Coordination**: Coordinate build infrastructure
  - Plan shared build server management
  - Coordinate build pipeline standardization
  - Design build artifact sharing
  - Plan build performance optimization

### Testing Infrastructure Coordination
- **Shared Testing Resources**: Coordinate testing infrastructure
  - Plan test environment provisioning
  - Coordinate test data management
  - Design test execution scheduling
  - Plan test result sharing

- **Quality Assurance Coordination**: Coordinate QA processes
  - Plan testing strategy coordination
  - Coordinate bug triage processes
  - Design quality metrics tracking
  - Plan quality improvement workflows

## Communication and Documentation Orchestration

### Documentation Collaboration
- **Collaborative Documentation**: Coordinate documentation efforts
  - Plan documentation ownership models
  - Coordinate documentation review processes
  - Design documentation update workflows
  - Plan documentation quality standards

- **Knowledge Base Management**: Coordinate knowledge repositories
  - Plan technical wiki management
  - Coordinate FAQ development
  - Design troubleshooting guide maintenance
  - Plan knowledge base search optimization

### Meeting and Synchronization Coordination
- **Regular Synchronization**: Coordinate team synchronization
  - Plan cross-team standup coordination
  - Coordinate technical sync meetings
  - Design architecture review meetings
  - Plan retrospective coordination

- **Decision Making Processes**: Coordinate decision workflows
  - Plan technical decision escalation
  - Coordinate consensus building processes
  - Design decision documentation workflows
  - Plan decision communication strategies

## Conflict Resolution and Escalation

### Technical Conflict Resolution
- **Design Disagreement Resolution**: Coordinate technical disputes
  - Plan technical arbitration processes
  - Coordinate expert consultation workflows
  - Design proof-of-concept evaluation
  - Plan decision appeal processes

- **Resource Conflict Management**: Coordinate resource disputes
  - Plan shared resource allocation
  - Coordinate priority conflict resolution
  - Design resource request workflows
  - Plan capacity planning coordination

### Process Improvement Coordination
- **Continuous Improvement**: Coordinate process enhancement
  - Plan retrospective coordination
  - Coordinate process experiment design
  - Design improvement implementation workflows
  - Plan improvement measurement strategies

## Onboarding and Knowledge Transfer

### New Team Member Integration
- **Onboarding Orchestration**: Coordinate new member integration
  - Plan technical onboarding workflows
  - Coordinate mentorship assignment
  - Design knowledge transfer processes
  - Plan skill assessment workflows

- **Cross-Team Knowledge Transfer**: Coordinate knowledge sharing
  - Plan expertise sharing sessions
  - Coordinate documentation handovers
  - Design knowledge validation processes
  - Plan knowledge retention strategies

### Skill Development Coordination
- **Team Skill Development**: Coordinate learning initiatives
  - Plan technical training coordination
  - Coordinate conference and learning opportunities
  - Design internal training programs
  - Plan skill gap analysis workflows

## Remote and Distributed Team Coordination

### Remote Collaboration Patterns
- **Distributed Development**: Coordinate remote team workflows
  - Plan asynchronous communication protocols
  - Coordinate time zone management
  - Design remote pair programming workflows
  - Plan remote meeting coordination

- **Cultural and Communication Coordination**: Manage diverse teams
  - Plan cross-cultural communication strategies
  - Coordinate language and communication standards
  - Design inclusive collaboration workflows
  - Plan cultural sensitivity training

### Tool and Platform Coordination
- **Collaboration Tool Management**: Coordinate collaboration platforms
  - Plan communication tool standardization
  - Coordinate project management tool usage
  - Design tool integration workflows
  - Plan tool training and support

## Performance and Productivity Coordination

### Team Performance Measurement
- **Productivity Metrics**: Coordinate performance measurement
  - Plan team velocity tracking
  - Coordinate quality metrics collection
  - Design performance improvement workflows
  - Plan productivity analysis processes

- **Bottleneck Identification**: Coordinate bottleneck resolution
  - Plan workflow bottleneck analysis
  - Coordinate process optimization
  - Design efficiency improvement workflows
  - Plan resource allocation optimization

### Workload Distribution Coordination
- **Load Balancing**: Coordinate workload distribution
  - Plan task distribution strategies
  - Coordinate expertise utilization
  - Design workload monitoring workflows
  - Plan capacity management processes

## Cross-References

### Memory Bank Integration
- **Framework Core**: [`framework-core.md`](../../memory-bank/main/framework-core.md:1)
- **Service Patterns**: [`service-patterns.md`](../../memory-bank/main/service-patterns.md:1)
- **Component System**: [`component-system.md`](../../memory-bank/main/component-system.md:1)
- **Troubleshooting**: [`troubleshooting-guide.md`](../../memory-bank/main/troubleshooting-guide.md:1)

### Specialized Domains
- **Advanced Monitoring**: [`monitoring-patterns.md`](../../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md:1)
- **Future Directions**: [`future-directions.md`](../../memory-bank/research/future-directions.md:1)
- **New Patterns**: [`new-patterns.md`](../../memory-bank/research/new-patterns.md:1)

### Related Mode Rules
- **Workflow Coordination**: [`workflow-coordination.md`](./workflow-coordination.md:1)
- **Complex Projects**: [`complex-projects.md`](./complex-projects.md:1)
- **Architect Mode**: [`system-design.md`](../architect/system-design.md:1)
- **Ask Mode**: [`framework-guidance.md`](../ask/framework-guidance.md:1)

## Team Collaboration Checklists

### Team Setup Checklist
- [ ] Team responsibilities clearly defined
- [ ] Communication protocols established
- [ ] Development workflows documented
- [ ] Code review processes implemented
- [ ] Documentation standards defined
- [ ] Onboarding processes created
- [ ] Conflict resolution procedures established

### Cross-Team Coordination Checklist
- [ ] Inter-team dependencies mapped
- [ ] Shared component ownership defined
- [ ] API contract collaboration established
- [ ] Technical decision processes implemented
- [ ] Knowledge sharing workflows active
- [ ] Regular synchronization meetings scheduled
- [ ] Escalation procedures documented

### Development Environment Checklist
- [ ] Standardized development environments deployed
- [ ] Shared build infrastructure operational
- [ ] Testing infrastructure coordinated
- [ ] Documentation systems accessible
- [ ] Communication tools configured
- [ ] Version control access granted
- [ ] Quality assurance processes active

### Performance and Quality Checklist
- [ ] Team performance metrics defined
- [ ] Quality standards established
- [ ] Productivity measurement systems active
- [ ] Bottleneck identification processes implemented
- [ ] Workload distribution optimized
- [ ] Continuous improvement processes active
- [ ] Skill development programs operational

### Remote Team Coordination Checklist
- [ ] Remote collaboration protocols established
- [ ] Asynchronous communication workflows defined
- [ ] Time zone coordination strategies implemented
- [ ] Cultural sensitivity guidelines established
- [ ] Remote meeting protocols defined
- [ ] Distributed development workflows operational
- [ ] Tool standardization completed