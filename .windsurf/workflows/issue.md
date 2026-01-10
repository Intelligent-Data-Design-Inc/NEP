# GitHub Issue Refinement and Planning Workflow

**PURPOSE: Refine the issue and generate an implementation plan. NO CODE IMPLEMENTATION.**

This workflow helps refine GitHub issues systematically to clarify requirements, generate a detailed implementation plan, and document findings on GitHub.

## Process Overview

**This workflow refines issues and generates implementation plans—no code implementation.**

1. **Issue Assessment & Validation**: Review and validate the GitHub issue
2. **Documentation Research**: Consult project documentation for architectural context
3. **Codebase Context**: Examine existing code patterns and similar implementations
4. **Requirements Refinement**: Ask targeted clarifying questions
5. **Dependency Analysis**: Check for dependencies and conflicts
6. **Implementation Plan Generation**: Create structured, actionable plan
7. **GitHub Documentation**: Record refined requirements and implementation plan

**The output is a refined issue with a clear implementation plan. Code implementation happens in a separate workflow.**

---

## Step 1: Issue Analysis & Validation

First, thoroughly examine the GitHub issue to understand:
- The problem description and context
- Current behavior vs. expected behavior
- Any error messages, logs, or screenshots provided
- Existing discussion or comments

**Validate issue quality:**
- ✓ Issue title is clear and specific
- ✓ Problem description is understandable
- ✓ Expected behavior is defined
- ✓ At least one reproduction step (for bugs)
- ✓ Environment details provided (if relevant)
- **If issue is incomplete:** Ask for missing information before proceeding

---

## Step 2: Consult Project Documentation

Review the project's documentation to gather architectural context and development guidance:
- **Examine docs/design.md** for system architecture, design patterns, and technical specifications
- **Check docs/prd.md** for product requirements, feature specifications, and business requirements
- **Review docs/prfaq.md** for frequently asked questions, common issues, and implementation guidance
- **Check README.md** for build instructions, dependencies, and project overview
- Look for relevant design decisions or technical specifications in the documentation
- Understand established patterns and conventions used in the project

---

## Step 3: Examine Codebase Context

**Search for similar implementations:**
- Look for related functionality in `src/`, `include/`, `test/`
- Identify existing NC_Dispatch implementations to follow
- Review error handling patterns and NetCDF error codes used
- Check memory management patterns (allocation, cleanup)
- Note build system patterns (CMake, Autotools)

**Check GitHub context:**
- Search for related existing issues (open and closed)
- Check for relevant pull requests
- Note any blocking or related issues
- Identify available milestones or project boards

---

## Step 4: Clarifying Questions

Ask 3-7 targeted questions to better understand the problem and requirements. Focus on:

**Technical Requirements:**
- What specific functionality needs to be implemented or fixed?
- Are there any existing code patterns, architectural constraints, or technical debt I should be aware of?
- What are the acceptance criteria for this issue?

**Context & Constraints:**
- What is the expected timeline and priority level?
- Are there any dependencies on other teams, systems, or components?
- What is the impact scope (users, systems, revenue, etc.)?

**Implementation Details:**
- Should this follow existing patterns in the codebase or introduce new approaches?
- Are there specific technologies, frameworks, or versions that must be used?
- What testing approach is expected (unit tests, integration tests, manual testing)?

Format questions as multiple choice with a recommended answer when appropriate.

---

## Step 5: Dependency Analysis

**Identify dependencies and conflicts:**
- Check if this issue depends on other open issues
- Look for potential conflicts with in-progress work
- Identify external dependencies (libraries, systems, teams)
- Note any blocking issues or prerequisites

**Document dependencies:**
- List dependent issues with issue numbers
- Identify any work that must be completed first
- Note any conflicts that need resolution

---

## Step 6: Implementation Plan Generation (NOT Implementation)

Based on the answers provided, generate a structured implementation plan:

**Plan Structure:**
```markdown
## Executive Summary
<2-3 sentence summary of problem and solution approach>

## Requirements & Acceptance Criteria
- [ ] <Specific, testable requirement 1>
- [ ] <Specific, testable requirement 2>
- [ ] <Specific, testable requirement 3>

## Implementation Approach
<High-level approach and rationale>

## Implementation Steps
1. <Concrete step 1> - <estimated effort>
2. <Concrete step 2> - <estimated effort>
3. <Concrete step 3> - <estimated effort>

## Dependencies
- Depends on #<issue_number> - <reason>
- Blocks #<issue_number> - <reason>

## Testing Requirements
- [ ] Unit tests for all new or modified functions
- [ ] Unit tests for error handling paths
- [ ] Integration test for <scenario>
- [ ] Manual testing for <edge cases>
- [ ] Test coverage verification (minimum 80% for new code)

## Risks & Mitigations
- <Risk> → <Mitigation strategy>

## Notes
<Additional context, references to existing code, etc.>
```

**Quality checks:**
- ✓ Steps are concrete and actionable
- ✓ Dependencies are clearly identified
- ✓ Testing requirements are specific
- ✓ Effort estimates are reasonable
- ✓ Plan follows project conventions

**Goal: Create a clear, actionable plan that can be executed in a separate implementation workflow.**

---

## Step 7: GitHub Documentation

Create a comprehensive follow-up comment on the GitHub issue that documents the refined requirements and implementation plan. This serves as both a record and a roadmap for future implementation.

**Comment structure:**
```markdown
## Issue Refinement Summary

### Executive Summary
<2-3 sentence recap of the core problem and the agreed solution approach>

### Requirements & Acceptance Criteria
- [ ] <Specific, testable requirement 1>
- [ ] <Specific, testable requirement 2>
- [ ] <Specific, testable requirement 3>

### Implementation Plan
<High-level approach and rationale>

#### Implementation Steps
1. <Concrete step 1> - <estimated effort>
2. <Concrete step 2> - <estimated effort>
3. <Concrete step 3> - <estimated effort>

### Dependencies
- Depends on #<issue_number> - <reason>
- Blocks #<issue_number> - <reason>

### Testing Requirements
- [ ] Unit tests for all new or modified functions
- [ ] Unit tests for error handling paths
- [ ] Integration test for <scenario>
- [ ] Manual testing for <edge cases>
- [ ] Test coverage verification (minimum 80% for new code)

### Risks & Mitigations
- <Risk> → <Mitigation strategy>

### Notes
<Additional context, references to existing code, etc.>
```

**After posting the comment:**
- Add relevant labels if needed (e.g., `ready-for-implementation`, `needs-design-review`)
- Link to related issues or documentation
- Consider adding to appropriate project board or milestone

**Result:** The issue is now well-documented with a clear implementation plan ready for execution.

## Important Constraints

- **ABSOLUTELY NO CODE CHANGES**: Do not modify, create, or delete any code files
- **ABSOLUTELY NO IMPLEMENTATION**: This workflow generates plans, not code
- **Focus on planning**: The goal is to refine the issue and create a detailed implementation plan
- **Record everything**: Document refined requirements and the implementation plan on GitHub
- **Implementation happens separately**: Use the /implement workflow to execute the generated plan

---

**WORKFLOW OUTPUT: A refined GitHub issue with clarified requirements and a detailed implementation plan ready for execution via /implement workflow.**

**Ready to begin. Please provide the GitHub issue URL or paste the issue content below:** 