# GitHub Issue Requirements Analysis Workflow

**PURPOSE: Requirements gathering and GitHub documentation ONLY. NO CODE CHANGES.**

This workflow helps analyze GitHub issues systematically to clarify requirements and document findings on GitHub.

## Process Overview

**This workflow is for analysis and planning ONLY—no code implementation.**

1. **Issue Assessment**: Review the GitHub issue to understand the core problem
2. **Wiki Research**: Consult the project wiki for architectural context and development guidance
3. **Requirements Gathering**: Ask targeted clarifying questions to fill knowledge gaps
4. **Solution Planning**: Synthesize answers into a clear action plan
5. **GitHub Documentation**: Record findings and next steps back on GitHub

**After this workflow completes, implementation happens separately.**

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

## Step 4: Solution Planning (NOT Implementation)

Based on the answers provided, create a plan for future implementation:
- Summarize the key requirements and constraints
- Identify the proposed approach and rationale
- Outline specific implementation steps in priority order (for later execution)
- Highlight any risks, assumptions, or dependencies

**Remember: You are planning what should be done, not doing it.**

## Step 5: GitHub Documentation

Create a comprehensive follow-up comment on the GitHub issue that serves as both a record and a roadmap. The comment should be structured with clear sections:

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

## Step 6: Final Review & Roadmap Creation

Review the complete analysis for completeness and accuracy, then append the finalized implementation roadmap to the bottom of the GitHub follow-up comment. The appended roadmap should include:

- A numbered list of concrete implementation steps (to be executed later)
- Owner/assignee for each major step
- Estimated completion dates
- Dependencies between steps
- Success criteria for each milestone

This ensures everyone has a clear, actionable roadmap that can be tracked directly from the GitHub issue.

**This roadmap documents what WILL be done in a future implementation phase. Do not execute these steps now.**

## Important Constraints

- **ABSOLUTELY NO CODE CHANGES**: Do not modify, create, or delete any code files
- **ABSOLUTELY NO IMPLEMENTATION**: This workflow is requirements gathering and documentation ONLY
- **Focus on clarity**: The goal is to understand what needs to be done, not to do it
- **Record everything**: Document the analysis and decisions back on GitHub for transparency
- **Implementation happens later**: After this workflow, a separate implementation phase will occur

---

**Ready to begin. Please provide the GitHub issue URL or paste the issue content below:** 