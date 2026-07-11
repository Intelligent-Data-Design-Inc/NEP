---
description: implement a roadmap sprint
auto_execution_mode: 3
---

# Sprint Implementation Workflow

This workflow implements a roadmap sprint by resolving its GitHub issue, extracting planning information, and following the structured implementation plan.

**Usage**: `/implement <version> sprint <number>`

**Example**: `/implement v2.4.0 sprint 1`

## Implementation Process

### Phase 1: Issue Analysis & Planning Extraction

1. **Resolve the sprint's GitHub issue**
   - Read `docs/roadmap.md` and locate the specified version and sprint
   - If the version or sprint is not found, report the error and stop
   - Extract the GitHub issue number/link recorded for the sprint
   - If no issue link is present, use `gh issue list --search "v{version} Sprint {n}:"` to find the issue
   - Record the issue number for all subsequent steps

2. **Fetch GitHub Issue Details**
   - Use `gh issue view <number>` to get issue body, labels, and metadata
   - Use `gh issue view --comments <number>` to retrieve all comments
   - Parse issue title, state, and assignee information

3. **Extract Implementation Plan from Issue**
   - Look for these sections in issue body and comments:
     - "Implementation Plan", "Implementation Steps", or "Implementation Roadmap"
     - "Requirements & Acceptance Criteria" 
     - "Technical Details" or "Technical Approach"
     - "Dependencies" and "Testing Requirements"
   - Extract numbered/bulleted task lists with time estimates
   - Identify acceptance criteria with checkboxes
   - Parse technical specifications and code examples

4. **Create Task Management Structure**
   - Initialize todo list with extracted implementation steps
   - Mark first step as `in_progress`
   - Include dependencies, testing, and documentation tasks

### Phase 2: Context & Architecture Review

5. **Review Project Documentation**
   - Read `docs/design.md` for architecture and design patterns
   - Read `docs/prd.md` for product requirements and specifications
   - Check for relevant format-specific documentation (e.g., GeoTIFF, CDF)

6. **Examine Codebase Context**
   - Locate relevant source files mentioned in the issue
   - Review existing implementation patterns
   - Identify similar functionality for reference
   - Check build system integration points

### Phase 3: Structured Implementation

7. **Execute Implementation Steps**
   For each step in the extracted plan:
   - **Code Implementation**: Write/modify code following existing patterns
   - **Documentation Updates**: Update relevant docs if specified
   - **Testing**: Implement unit/integration tests as required
   - **Validation**: Verify acceptance criteria are met
   - **Progress Tracking**: Mark step complete, update todo list

8. **Handle Dependencies**
   - Verify all dependencies are satisfied before implementation
   - Check for blocking issues or prerequisites
   - Ensure build system changes are consistent across CMake/Autotools

### Phase 4: Quality Assurance & Completion

9. **Testing & Validation**
   - Run unit tests for new/modified code
   - Execute integration tests with sample data
   - Verify no regression in existing functionality
   - Check memory management and error handling

10. **Definition of Done Verification**
   - All acceptance criteria met
   - Tests passing with required coverage
   - Documentation updated
   - No regressions introduced
   - Code follows project patterns

11. **Final Integration**
    - Run full test suite using `/btest` workflow
    - Use `gh issue comment <number>` to update the issue with implementation status
    - Use `gh issue edit <number>` or `gh issue close <number>` to mark it ready for review/close if appropriate

## Implementation Guidelines

### Code Quality Standards
- Follow existing code style and patterns
- Maintain backward compatibility
- Use established error handling patterns
- Ensure proper memory management
- Add appropriate logging and debugging support

### Testing Requirements
- Unit tests for all new functions
- Integration tests for new features
- Error handling path testing
- Memory leak validation
- Performance testing if applicable

### Documentation Standards
- Update API documentation for new functions
- Add inline comments for complex logic
- Update design documents if architecture changes
- Include usage examples in relevant docs

## Error Handling

- If the version or sprint is not found in `docs/roadmap.md`, report the error and stop
- If the sprint has no GitHub issue, redirect to `/roadmap <version> sprint <number>` first
- If `gh` CLI fails, retry once and then report the error; do not fall back to MCP GitHub tools on this network
- If issue lacks clear implementation plan, redirect to `/issue` workflow first
- If dependencies are missing, document and block implementation
- If tests fail, debug and fix before proceeding
- If acceptance criteria unclear, ask for clarification on the GitHub issue

## Progress Tracking

- Use todo_list to track implementation steps
- Use `gh issue comment <number>` to add progress comments to the GitHub issue
- Mark tasks complete as they are finished
- Document any deviations from the original plan

## Example Implementation Flow

For `/implement v2.4.0 sprint 1`:
1. Read docs/roadmap.md → Resolve GitHub issue for v2.4.0 Sprint 1
2. Fetch issue → Extract implementation plan
3. Review docs/design.md → Understand relevant architecture
4. Examine source files → Locate code to change
5. Implement steps → Run tests → Update GitHub

Example usage: `/implement v2.4.0 sprint 1`