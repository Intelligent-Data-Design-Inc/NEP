---
description: Plan a sprint.
auto_execution_mode: 3
---

# Sprint Planning Workflow

Create a detailed implementation plan for a sprint.

## Usage

```
/plan <version> sprint <number>
```

Example: `/plan v0.5.0 sprint 2`

## Steps

1. **Read expanded roadmap requirements**
   - Check `docs/roadmap.md` for sprint section
   - Review implementation decisions from roadmap expansion
   - Check for existing expanded requirements in `docs/plans/`

2. **Break down into implementation phases**
   - Group related tasks into phases (typically 3-5 phases)
   - Order phases by dependencies and priority
   - Each phase should have clear objective

3. **Create detailed task breakdown**
   - Break each phase into specific tasks
   - Include file paths, code structure, and requirements
   - Add acceptance criteria for each task
   - Specify test requirements

4. **Document technical details**
   - Add code examples and patterns to follow
   - Document error codes and responses
   - Include testing strategies
   - Note risk areas

5. **Define success metrics**
   - Test coverage targets
   - Performance requirements
   - Error rate thresholds
   - Security requirements

6. **Create implementation plan file**
   - Save to `docs/plan/<version>-sprint<number>.md`
   - Use consistent format with Overview, Phases, Definition of Done
   - Keep concise but comprehensive
   - Include quick reference section

7. **Implement the Plan**
   - Execute the implement workflow for this version and sprint

## Plan Structure

```markdown
# <Version> Sprint <Number> Implementation Plan

## Overview
- Goal, duration, deliverables

## Prerequisites
- What must be complete before starting

## Phase N: <Name>
### Task N.1: <Task Name>
- File paths
- Requirements
- Acceptance criteria

## Definition of Done
- Checklist of completion criteria

## Technical Notes
- Quick reference information

## Risk Assessment
- Potential issues and mitigations

## Success Metrics
- Measurable targets
```

## Notes

- Plans should be implementation-ready
- Include enough detail for /implement workflow
- Balance detail with conciseness
- Focus on "what" and "how", not "why" (that's in roadmap)