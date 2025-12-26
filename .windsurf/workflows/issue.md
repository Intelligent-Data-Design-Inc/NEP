# GitHub Issue Refinement and Planning Workflow

**PURPOSE: Refine the issue and generate an implementation plan. NO CODE IMPLEMENTATION.**

This workflow helps refine GitHub issues systematically to clarify requirements, generate a detailed implementation plan, and document findings on GitHub.

## Process Overview

**This workflow refines issues and generates implementation plans—no code implementation.**

1. **Issue Assessment**: Review the GitHub issue to understand the core problem
2. **Documentation Research**: Consult project documentation for architectural context and development guidance
3. **Requirements Refinement**: Ask targeted clarifying questions to fill knowledge gaps
4. **Implementation Plan Generation**: Create a detailed, actionable plan for future implementation
5. **GitHub Documentation**: Record refined requirements and implementation plan on GitHub

**The output is a refined issue with a clear implementation plan. Code implementation happens in a separate workflow.**

## Step 1: Issue Analysis

First, thoroughly examine the GitHub issue to understand:
- The problem description and context
- Current behavior vs. expected behavior
- Any error messages, logs, or screenshots provided
- Existing discussion or comments

## Step 2: Consult Project Documentation

Review the project's documentation to gather architectural context and development guidance:
- **Examine docs/design.md** for system architecture, design patterns, and technical specifications
- **Check docs/prd.md** for product requirements, feature specifications, and business requirements
- **Review docs/prfaq.md** for frequently asked questions, common issues, and implementation guidance
- Look for relevant design decisions or technical specifications in the documentation
- Understand established patterns and conventions used in the project
- Review any setup, deployment, or configuration guidelines

## Step 3: Clarifying Questions

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

## Step 4: Implementation Plan Generation (NOT Implementation)

Based on the answers provided, generate a detailed implementation plan:
- Summarize the key requirements and constraints
- Identify the proposed approach and rationale
- Outline specific implementation steps in priority order (for later execution)
- Highlight any risks, assumptions, or dependencies
- Estimate effort and identify dependencies between steps

**Goal: Create a clear, actionable plan that can be executed in a separate implementation workflow.**

## Step 5: GitHub Documentation

Create a comprehensive follow-up comment on the GitHub issue that documents the refined requirements and implementation plan. This serves as both a record and a roadmap for future implementation. The comment should be structured with clear sections:

**Executive Summary:**
- 2-3 sentence recap of the core problem and the agreed solution approach
- Brief mention of the expected outcome and business value

**Requirements & Acceptance Criteria:**
- Clear, testable requirements in bullet point format
- Specific acceptance criteria that define "done"
- Any non-functional requirements (performance, security, usability)

**Technical Implementation Plan:**
- Detailed step-by-step approach with logical grouping
- Estimated effort for each major component (story points or time)
- Key technical decisions and their rationale
- Integration points with existing systems

**Open Questions & Decision Points:**
- Outstanding items that need stakeholder input
- Decisions that need to be made before implementation
- Areas requiring further research or prototyping

**Posting to GitHub:**
Use the `gh` command line tool to post the comment to the issue:
```bash
gh issue comment <issue_number> --body "your comment text here"
```
This ensures the comment is properly posted even if MCP GitHub tools have permission issues.

## Step 6: Final Review & Implementation Roadmap

Review the complete analysis for completeness and accuracy, then append the finalized implementation roadmap to the bottom of the GitHub follow-up comment. The roadmap should include:

- A numbered list of concrete implementation steps (to be executed in a separate workflow)
- Owner/assignee for each major step
- Estimated completion dates or effort
- Dependencies between steps
- Success criteria for each milestone

This ensures everyone has a clear, actionable roadmap that can be tracked directly from the GitHub issue.

**This roadmap is the PRIMARY OUTPUT of this workflow. It documents what WILL be done when the implementation workflow is executed. Do not implement these steps now.**

## Important Constraints

- **ABSOLUTELY NO CODE CHANGES**: Do not modify, create, or delete any code files
- **ABSOLUTELY NO IMPLEMENTATION**: This workflow generates plans, not code
- **Focus on planning**: The goal is to refine the issue and create a detailed implementation plan
- **Record everything**: Document refined requirements and the implementation plan on GitHub
- **Implementation happens separately**: Use the /implement workflow to execute the generated plan

---

**WORKFLOW OUTPUT: A refined GitHub issue with clarified requirements and a detailed implementation plan ready for execution via /implement workflow.**

**Ready to begin. Please provide the GitHub issue URL or paste the issue content below:** 