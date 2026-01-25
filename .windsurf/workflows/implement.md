---
description: implement an issue
auto_execution_mode: 3
---

# Issue Implementation Workflow

This workflow implements GitHub issues by extracting planning information and following structured implementation plans.

**Usage**: `/implement [issue_number]` or `Implement issue [issue_number]`

## Implementation Process

### Phase 1: Issue Analysis & Planning Extraction

1. **Fetch GitHub Issue Details**
   - Use `mcp0_issue_read` to get issue body, labels, and metadata
   - Use `mcp0_issue_read` with `get_comments` to retrieve all comments
   - Parse issue title, state, and assignee information

2. **Extract Implementation Plan from Issue**
   - Look for these sections in issue body and comments:
     - "Implementation Plan", "Implementation Steps", or "Implementation Roadmap"
     - "Requirements & Acceptance Criteria" 
     - "Technical Details" or "Technical Approach"
     - "Dependencies" and "Testing Requirements"
   - Extract numbered/bulleted task lists with time estimates
   - Identify acceptance criteria with checkboxes
   - Parse technical specifications and code examples

3. **Create Task Management Structure**
   - Initialize todo list with extracted implementation steps
   - Mark first step as `in_progress`
   - Include dependencies, testing, and documentation tasks

### Phase 2: Context & Architecture Review

4. **Review Project Documentation**
   - Read `docs/design.md` for architecture and design patterns
   - Read `docs/prd.md` for product requirements and specifications
   - Check for relevant format-specific documentation (e.g., GeoTIFF, CDF)

5. **Examine Codebase Context**
   - Locate relevant source files mentioned in the issue
   - Review existing implementation patterns
   - Identify similar functionality for reference
   - Check build system integration points

### Phase 3: Structured Implementation

6. **Execute Implementation Steps**
   For each step in the extracted plan:
   - **Code Implementation**: Write/modify code following existing patterns
   - **Documentation Updates**: Update relevant docs if specified
   - **Testing**: Implement unit/integration tests as required
   - **Validation**: Verify acceptance criteria are met
   - **Progress Tracking**: Mark step complete, update todo list

7. **Handle Dependencies**
   - Verify all dependencies are satisfied before implementation
   - Check for blocking issues or prerequisites
   - Ensure build system changes are consistent across CMake/Autotools

### Phase 4: Quality Assurance & Completion

8. **Testing & Validation**
   - Run unit tests for new/modified code
   - Execute integration tests with sample data
   - Verify no regression in existing functionality
   - Check memory management and error handling

9. **Definition of Done Verification**
   - All acceptance criteria met
   - Tests passing with required coverage
   - Documentation updated
   - No regressions introduced
   - Code follows project patterns

10. **Final Integration**
    - Run full test suite using `/btest` workflow
    - Update GitHub issue with implementation status
    - Mark issue as ready for review/close if appropriate

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

- If issue lacks clear implementation plan, redirect to `/issue` workflow first
- If dependencies are missing, document and block implementation
- If tests fail, debug and fix before proceeding
- If acceptance criteria unclear, ask for clarification on GitHub issue

## Progress Tracking

- Use todo_list to track implementation steps
- Update GitHub issue with progress comments
- Mark tasks complete as they are finished
- Document any deviations from the original plan

## Example Implementation Flow

For issue 92 (CRS Metadata Extraction):
1. Fetch issue → Extract 7-step implementation plan
2. Review docs/design.md → Understand GeoTIFF architecture
3. Examine src/geotifffile.c → Locate placeholder code
4. Implement Step 1: Add CRS data structures → Update header
5. Implement Step 2: extract_crs_parameters() → Write function
6. Continue through all steps → Run tests → Update GitHub

Example usage: "/implement 60" or "Implement issue 60"