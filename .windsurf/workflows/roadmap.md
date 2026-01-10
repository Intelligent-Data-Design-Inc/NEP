---
description: Expand roadmap sprint into detailed planning document
---

# Roadmap Sprint Expansion Workflow

This workflow creates detailed sprint planning documents from roadmap entries.

## Usage

```
/roadmap <version> sprint <number>
```

**Example:** `/roadmap v0.4.0 sprint 3`

**Output:** Updates `docs/roadmap.md` with clarified sprint details

---

## Scope

**Do:**
- Expand and clarify tasks from the specified sprint
- Identify technical gaps and missing requirements
- Update roadmap and related documentation with clarifications

**Don't:**
- Modify other versions or sprints
- Implement code (save for `/plan` workflow)
- Create new sprints or features not in the roadmap

---

## Steps

### 1. Validate sprint exists

- Read `docs/roadmap.md` and verify the specified version exists
- Confirm the sprint number exists within that version
- **If not found:** Report error and stop workflow

### 2. Extract sprint details from roadmap

- Locate the specified version and sprint number in `docs/roadmap.md`
- Extract all tasks, goals, and definition of done criteria
- Identify dependencies on previous sprints or external systems
- Note any incomplete or ambiguous requirements

### 3. Review related documentation for context

- Check `docs/prd.md` for product requirements and functional specifications
- Review `docs/design.md` for architecture and technical design
- Review existing sprint plans in `docs/plan/` to understand format and detail level
- Check source code in `src/`, `include/`, and `test/` for existing patterns

### 4. Analyze sprint scope and identify gaps

- Review each task for completeness and clarity
- Identify missing implementation details (NC_Dispatch functions, data structures, error handling)
- Check for missing testing requirements (C unit tests, Fortran tests, integration tests)
- Verify build system integration (CMake and Autotools)
- Identify dependency requirements and version constraints
- Check if dependencies from previous sprints are satisfied

### 5. Ask 3-6 numbered clarifying questions

**Focus areas:**
- Ambiguous requirements or missing technical details
- Error handling strategies and NetCDF error code mapping
- Testing expectations and coverage targets
- Build system integration and dependency detection
- Memory management and resource cleanup
- Integration with native format libraries (NCEPLIBS-g2, libgeotiff, NASA CDF)

**Question format:**
- Provide suggested answers, labeled with letters, so questions can be answered with just a letter
- Recommend an answer and provide justification for your recommendation
- Ask minimum 3 questions, maximum 6 questions

**⚠️ ASK QUESTIONS NOW**

**Wait for user responses before proceeding**

**If answers are incomplete:**
- Ask follow-up questions to clarify
- Maximum 2 rounds of questions total

### 6. Update roadmap with clarifications

- Incorporate answers from step 5 into `docs/roadmap.md`
- Add implementation decisions to sprint section
- Clarify ambiguous tasks with specific details
- Add any new subtasks or requirements discovered
- Update definition of done if needed

**Important:**
- **DO NOT** add proposed code changes to the roadmap
- Code can be added to the plan (next step)
- If example code is presented by the user, that may be included in the roadmap

### 7. Update related documentation (if needed)

**Only update if clarifications revealed:**
- Architecture changes → Update `docs/design.md`
- Functional requirement changes → Update `docs/prd.md`
- NC_Dispatch or UDF handler design notes → Add to `docs/design.md`

**Skip this step if:** No architectural or functional changes were clarified

### 8. Verify completion criteria

**Confirm all items are satisfied:**
- ✓ All sprint tasks are clearly defined
- ✓ Implementation approach is specified for each task
- ✓ Testing requirements are identified
- ✓ Build system integration is addressed
- ✓ Dependencies are documented
- ✓ Error handling strategy is defined
- ✓ Roadmap is updated with all clarifications

**If any criteria are missing:** Return to step 5 and ask additional questions

**Next step:** User can run `/plan <version> sprint <number>` to create detailed implementation plan

---

## Error Handling

**If sprint not found in roadmap:**
- Report: "Sprint {number} not found in version {version} of docs/roadmap.md"
- List available sprints for that version
- Stop workflow

**If dependencies not met:**
- Report: "Previous sprint dependencies not satisfied"
- List missing dependencies
- Ask user if they want to proceed anyway

**If roadmap is malformed:**
- Report specific formatting issues found
- Ask user to fix roadmap structure first
- Stop workflow
