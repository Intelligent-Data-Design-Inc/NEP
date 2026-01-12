---
description: Refine GitHub issues and generate implementation plans
---

# Issue Refinement Workflow

This workflow helps refine GitHub issues systematically to clarify requirements, identify dependencies, and generate actionable implementation plans.

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

**For bug reports:**
- Reproduction steps and environment details
- Expected vs. actual behavior
- Error messages or logs
- When the issue started occurring
- Any recent changes that might be related

**For feature requests:**
- Specific use cases and workflows
- Performance requirements or constraints
- Integration with existing functionality
- User interface considerations
- Backward compatibility requirements

**For documentation issues:**
- Which documentation needs updating
- Target audience (users, developers, contributors)
- Missing or unclear information
- Examples that would be helpful

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
<Brief 2-3 sentence summary of the core problem and proposed solution approach>

## Requirements & Acceptance Criteria
- [ ] <Specific, testable requirement 1>
- [ ] <Specific, testable requirement 2>
- [ ] <Specific, testable requirement 3>

## Implementation Approach
<High-level technical approach and rationale for the chosen solution>

## Implementation Steps
1. <Concrete implementation step 1> - <estimated effort in hours/story points>
2. <Concrete implementation step 2> - <estimated effort in hours/story points>
3. <Concrete implementation step 3> - <estimated effort in hours/story points>

## Dependencies
- Depends on #<issue_number> - <reason for dependency>
- Blocks #<issue_number> - <reason for blocking>

## Testing Requirements
- [ ] Unit tests for all new or modified functions
- [ ] Unit tests for error handling paths
- [ ] Integration test for <specific scenario>
- [ ] Manual testing for <specific edge cases>
- [ ] Test coverage verification (minimum 80% for new code)

## Risks & Mitigations
- <Identified risk> → <Specific mitigation strategy>

## Notes
<Additional context, references to existing code patterns, etc.>
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
<Brief 2-3 sentence recap of the core problem and the agreed solution approach>

### Requirements & Acceptance Criteria
- [ ] <Specific, testable requirement 1>
- [ ] <Specific, testable requirement 2>
- [ ] <Specific, testable requirement 3>

### Implementation Plan
<High-level approach and rationale>

### Next Steps
<Immediate actions needed to proceed with implementation>
```

**Posting to GitHub:**
Use the `gh` command line tool to post the comment to the issue:
```bash
gh issue comment <issue_number> --body "your comment text here"
```

---

## Step 8: Final Review & Implementation Roadmap

Review the complete analysis for completeness and accuracy, then append the finalized implementation roadmap to the bottom of the GitHub follow-up comment. The roadmap should include:

- A numbered list of concrete implementation steps (to be executed in a separate workflow)
- Owner/assignee for each major step
- Estimated completion dates or effort
- Dependencies between steps
- Success criteria for each milestone

**Final validation:**
- ✓ All questions have been answered
- ✓ Requirements are clear and testable
- ✓ Implementation plan is technically sound
- ✓ Dependencies are documented
- ✓ Testing strategy is comprehensive
