---
description: Plan a sprint by creating GitHub issues
auto_execution_mode: 3
---

# Sprint Planning Workflow

Create 1-7 GitHub issues for a sprint, each containing one actionable change.

## Usage

```
/plan <version> sprint <number>
```

**Example:** `/plan v0.5.0 sprint 2`

**Output:** Creates 1-7 GitHub issues with labels `<version>` and `sprint-<number>`

---

## Scope

**Each issue must:**
- Contain ONE actionable change to the codebase
- Include specific file paths and functions to modify
- Have clear acceptance criteria
- Specify dependencies on other issues (if any)
- Include testing requirements

**Issue count:** Minimum 1, maximum 7 per sprint

---

## Steps

### 1. Read and validate roadmap requirements

- Check `docs/roadmap.md` for the specified sprint section
- **If sprint not found:** Report error and stop workflow
- Extract all tasks, goals, and definition of done criteria

**Validate roadmap completeness:**
- Each task has clear implementation approach
- Testing requirements are specified
- Dependencies are identified
- Error handling is addressed
- **If insufficient detail:** Report gaps and suggest running `/roadmap <version> sprint <number>` first

**Review supporting documentation:**
- `README.md` - Project overview, build instructions, dependencies
- `docs/design.md` - Architecture and technical design
- `docs/prd.md` - Product requirements and functional specifications
- `docs/prfaq.md` - User impact, use cases, and compatibility requirements

### 2. Review existing codebase and GitHub context

**Examine existing code patterns:**
- Search for similar implementations in `src/`, `include/`, `test/`
- Identify existing NC_Dispatch implementations to follow
- Review error handling patterns and NetCDF error codes used
- Check memory management patterns (allocation, cleanup)
- Note build system patterns (CMake, Autotools)

**Check GitHub context:**
- Search for related existing issues (open and closed)
- Check for relevant pull requests
- Identify available milestones for this version
- Note any blocking or related issues

### 3. Break down into actionable changes and classify types

- Identify 1-7 distinct, implementable changes
- Each change should be completable independently (or with clear dependencies)
- Order by dependencies and priority
- Group related file changes into single issues when logical

**Classify each change by type:**

**Feature Issues:**
- New functionality, API additions, format support
- Template: Focus on implementation, testing, documentation
- Example: Add NC_Dispatch function for GeoTIFF metadata

**Bug Issues:**
- Fix incorrect behavior, crashes, memory leaks
- Template: Focus on reproduction, root cause, fix validation
- Example: Fix memory leak in variable cleanup

**Refactor Issues:**
- Code organization, performance improvements, cleanup
- Template: Focus on before/after, migration path, compatibility
- Example: Consolidate error handling across modules

**Test Issues:**
- New test suites, coverage improvements, test infrastructure
- Template: Focus on test scenarios, coverage, automation
- Example: Add comprehensive tests for NC_VAR_INFO_T

**Build Issues:**
- Build system updates, dependency management, CI/CD
- Template: Focus on build verification, compatibility, automation
- Example: Update CMake for new NetCDF-C dependency

**Issue sizing guidelines:**

**Good size (1-4 hours):**
- Feature: Add single NC_Dispatch function implementation
- Bug: Fix specific memory leak or crash
- Refactor: Reorganize one module's error handling
- Test: Add test suite for one feature
- Build: Update build system for one dependency

**Too small (<30 min):**
- Fix single typo or comment
- Add one test case
- Update single constant

**Too large (>4 hours):**
- Feature: Implement entire format support
- Bug: Fix systemic memory issues across modules
- Refactor: Refactor multiple modules
- Test: Add comprehensive test coverage for whole system
- Build: Overhaul entire build system

**If change is too large:** Break into multiple issues with dependencies

### 4. Define dependencies between changes

- Identify which changes must happen before others
- Create dependency graph (Issue A → Issue B → Issue C)
- Ensure no circular dependencies
- Note shared dependencies (e.g., multiple issues need same data structure)

**Dependency order determines creation order** - Create issues in dependency order so earlier issues can be referenced by number

### 5. Prepare all issue content with type-specific templates

For each of the 1-7 changes, prepare complete issue content **before creating any issues**.

**Title format:** `[<version>] <type>: <Brief description of change>`

**Examples:**
- `[v0.5.0] feature: Add NC_Dispatch function for GeoTIFF metadata`
- `[v0.5.0] bug: Fix memory leak in variable cleanup`
- `[v0.5.0] refactor: Consolidate error handling across modules`

**Body structure by type:**

**Feature Issue Template:**
```markdown
## Goal
<What new functionality this provides and why it's needed>

## Files to Modify
- `path/to/file1.c` - <specific changes>
- `path/to/file2.h` - <specific changes>

## Implementation Plan
1. <Concrete step 1>
2. <Concrete step 2>
3. <Concrete step 3>

## Testing Requirements
- [ ] Unit tests for all new functions
- [ ] Unit tests for error handling paths
- [ ] Integration test for end-to-end functionality
- [ ] Fortran test for API compatibility
- [ ] Documentation tests/examples
- [ ] Test coverage verification (minimum 80% for new code)

## Acceptance Criteria
- [ ] <Specific, testable criterion 1>
- [ ] <Specific, testable criterion 2>
- [ ] All tests pass
- [ ] No memory leaks (valgrind clean)
- [ ] Documentation updated
- [ ] Unit test coverage meets minimum requirements

## Dependencies
- Depends on #<issue_number> - <reason>
- Blocks #<issue_number> - <reason>

## Technical Notes
- Code patterns to follow: <reference to existing code>
- Error codes: <specific NC_E* codes to use>
- Design decisions: <architectural notes>

## Risks
- <Potential issue> → <mitigation strategy>
```

**Bug Issue Template:**
```markdown
## Goal
<Fix the incorrect behavior and prevent regression>

## Problem Description
<What is broken and how it manifests>
- **Symptoms:** <observable effects>
- **Root cause:** <underlying issue>
- **Impact:** <who/what is affected>

## Files to Modify
- `path/to/file1.c` - <specific changes>
- `path/to/file2.h` - <specific changes>

## Implementation Plan
1. <Concrete step 1>
2. <Concrete step 2>
3. <Concrete step 3>

## Testing Requirements
- [ ] Unit tests for modified functions
- [ ] Unit tests for error handling paths
- [ ] Reproduction test for the bug
- [ ] Regression test to prevent reoccurrence
- [ ] Integration test for affected scenarios
- [ ] Test coverage verification (minimum 80% for modified code)

## Acceptance Criteria
- [ ] Bug no longer reproduces
- [ ] No regression in existing functionality
- [ ] All tests pass
- [ ] No memory leaks (valgrind clean)
- [ ] Unit test coverage meets minimum requirements

## Dependencies
- Depends on #<issue_number> - <reason>
- Blocks #<issue_number> - <reason>

## Technical Notes
- Error codes: <specific NC_E* codes to use>
- Debugging approach: <how to verify fix>

## Risks
- <Potential issue> → <mitigation strategy>
```

**Refactor Issue Template:**
```markdown
## Goal
<Improve code organization, performance, or maintainability>

## Current State
<What needs to be improved and why>
- **Problems:** <current issues>
- **Performance impact:** <if applicable>
- **Maintenance burden:** <if applicable>

## Proposed Changes
<What the refactoring will accomplish>
- **Before:** <current state>
- **After:** <desired state>
- **Migration path:** <how to transition>

## Files to Modify
- `path/to/file1.c` - <specific changes>
- `path/to/file2.h` - <specific changes>

## Implementation Plan
1. <Concrete step 1>
2. <Concrete step 2>
3. <Concrete step 3>

## Testing Requirements
- [ ] Unit tests for all modified functions
- [ ] Unit tests for error handling paths
- [ ] Tests for refactored components
- [ ] Compatibility tests for existing APIs
- [ ] Performance tests (if applicable)
- [ ] Test coverage verification (minimum 80% for modified code)

## Acceptance Criteria
- [ ] Refactoring completed successfully
- [ ] No breaking changes to public APIs
- [ ] Performance improved (if applicable)
- [ ] All tests pass
- [ ] No memory leaks (valgrind clean)
- [ ] Unit test coverage meets minimum requirements

## Dependencies
- Depends on #<issue_number> - <reason>
- Blocks #<issue_number> - <reason>

## Technical Notes
- Backward compatibility: <how maintained>
- Migration notes: <for users>

## Risks
- <Potential issue> → <mitigation strategy>
```

**Test Issue Template:**
```markdown
## Goal
<Improve test coverage or add test infrastructure>

## Testing Scope
<What will be tested and why>
- **Coverage gap:** <what's currently missing>
- **Risk area:** <what needs protection>
- **Test type:** <unit/integration/performance>

## Files to Modify
- `test/test_file1.c` - <specific changes>
- `test/test_file2.c` - <specific changes>
- `src/source_file.c` - <if test fixtures needed>

## Implementation Plan
1. <Concrete step 1>
2. <Concrete step 2>
3. <Concrete step 3>

## Testing Requirements
- [ ] Unit tests for all new functions
- [ ] Unit tests for error handling paths
- [ ] Integration tests for component interactions
- [ ] Performance tests (if applicable)
- [ ] Test coverage verification (minimum 80% for new code)
- [ ] CI/CD pipeline integration

## Acceptance Criteria
- [ ] Test coverage increased by <X>%
- [ ] All new tests pass
- [ ] Tests run in CI/CD pipeline
- [ ] Tests are maintainable and clear
- [ ] Unit test coverage meets minimum requirements

## Dependencies
- Depends on #<issue_number> - <reason>
- Blocks #<issue_number> - <reason>

## Technical Notes
- Test framework: <CUnit, etc.>
- Test data: <fixtures, mocks>
- Coverage tools: <gcov, etc.>

## Risks
- <Potential issue> → <mitigation strategy>
```

**Build Issue Template:**
```markdown
## Goal
<Improve build system, dependencies, or CI/CD>

## Current State
<What needs improvement and why>
- **Problems:** <current build issues>
- **Compatibility:** <platform/dependency issues>
- **Performance:** <build speed, size, etc.>

## Proposed Changes
<What the build changes will accomplish>

## Files to Modify
- `CMakeLists.txt` - <specific changes>
- `configure.ac` - <specific changes>
- `.github/workflows/*.yml` - <specific changes>

## Implementation Plan
1. <Concrete step 1>
2. <Concrete step 2>
3. <Concrete step 3>

## Testing Requirements
- [ ] Unit tests for build system changes
- [ ] Build verification on all platforms
- [ ] Dependency compatibility tests
- [ ] CI/CD pipeline tests
- [ ] Test coverage verification for build scripts (minimum 70%)

## Acceptance Criteria
- [ ] Build succeeds on all target platforms
- [ ] Dependencies properly resolved
- [ ] CI/CD pipeline updated and working
- [ ] Documentation updated
- [ ] Unit test coverage meets minimum requirements

## Dependencies
- Depends on #<issue_number> - <reason>
- Blocks #<issue_number> - <reason>

## Technical Notes
- Build tools: <versions, requirements>
- Platform compatibility: <Linux, macOS, Windows>
- Dependency versions: <specific versions>

## Risks
- <Potential issue> → <mitigation strategy>
```

**Quality check each issue:**
- ✓ Title includes type: `[<version>] <type>: <description>`
- ✓ Goal explains what AND why
- ✓ File paths are specific and accurate
- ✓ Implementation steps are concrete, not vague
- ✓ Acceptance criteria are testable
- ✓ Dependencies are correctly identified
- ✓ Follows existing code patterns

**Type-specific validation:**
- **Feature:** Includes documentation requirements, API compatibility, unit tests for new functions
- **Bug:** Includes problem description, reproduction steps, regression prevention, unit tests for modified functions
- **Refactor:** Includes current state, proposed changes, migration path, unit tests for modified functions
- **Test:** Includes coverage targets, test framework details, unit tests for test infrastructure
- **Build:** Includes platform compatibility, dependency versions, unit tests for build changes

### 6. Create GitHub issues in dependency order

**Create issues sequentially in dependency order:**

1. Create foundation issues first (no dependencies)
2. Note each created issue number
3. Update prepared content for dependent issues with actual issue numbers
4. Create next tier of issues
5. Repeat until all issues created

**For each issue:**
- Use `mcp0_issue_write` tool with method `create`
- Set title: `[<version>] <description>`
- Set body with prepared content (updated with actual issue numbers)
- Add labels: `<version>`, `sprint-<number>`
- **Do not assign** issues yet (user will assign)

### 7. Verify issue set completeness

**Confirm all items are covered:**
- ✓ All roadmap tasks are represented in issues
- ✓ Dependencies are correctly specified
- ✓ No circular dependencies exist
- ✓ Each issue is actionable and scoped
- ✓ Testing requirements are specified
- ✓ All issues have correct labels

**If gaps exist:** Create additional issues (up to 7 total)

### 8. Report summary

Provide summary with:
- Total issues created
- Issue numbers and titles
- Dependency order (which to implement first)
- Estimated total effort
- Link to GitHub issues page

---

## Issue Dependency Format

**In issue body, use:**
```markdown
## Dependencies
- Depends on #123 - Need NC_VAR_INFO_T structure defined
- Blocks #125 - This must complete before error handling
```

**Dependency types:**
- **Depends on:** This issue cannot start until dependency is complete
- **Blocks:** This issue must complete before blocked issue can start
- **Related to:** Shares context but no hard dependency

---

## Error Handling

**If sprint not found in roadmap:**
- Report: "Sprint {number} not found in version {version} of docs/roadmap.md"
- List available sprints for that version
- Stop workflow

**If roadmap sprint is incomplete:**
- Report: "Sprint {number} in {version} lacks sufficient detail"
- Suggest running `/roadmap <version> sprint <number>` first
- Stop workflow

**If more than 7 changes needed:**
- Report: "Sprint requires more than 7 issues - consider splitting into multiple sprints"
- Ask user which tasks to prioritize for this sprint
- Wait for response before creating issues