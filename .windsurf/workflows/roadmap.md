---
description: Break version work into detailed sprint planning documents
---

# Version Roadmap Planning Workflow

This workflow breaks down a version's work into 1-5 detailed sprint planning documents.

## Usage

```
/roadmap <version>
```

**Example:** `/roadmap v2.0.0`

**Output:** Updates `docs/roadmap.md` with clarified sprint breakdown for the specified version

---

## Scope

**Do:**
- Analyze the specified version's work and break it into 1-5 sprints
- Identify technical gaps and missing requirements across all sprints
- Update roadmap and related documentation with clarifications
- Ensure logical dependency flow between sprints

**Don't:**
- Modify other versions not specified
- Implement code (save for `/plan` workflow)
- Create more than 5 sprints for a version
- Skip dependency analysis between sprints

---

## Steps

### 1. Validate version exists

- Read `docs/roadmap.md` and verify the specified version exists
- **If not found:** Report error and stop workflow

### 2. Extract version details from roadmap

- Locate the specified version in `docs/roadmap.md`
- Extract all existing work items, goals, and requirements
- Identify any existing sprint breakdown
- Note incomplete or ambiguous requirements
- Assess total scope and complexity of the version

### 3. Review related documentation for context

- Check `docs/prd.md` for product requirements and functional specifications
- Review `docs/design.md` for architecture and technical design
- Review existing sprint plans in `docs/plan/` to understand format and detail level
- Check source code in `src/`, `include/`, and `test/` for existing patterns

### 4. Analyze version scope and identify optimal sprint breakdown

- Review all work items for completeness and clarity
- Group related tasks into logical sprint units (1-5 sprints total)
- Identify dependencies between task groups
- Ensure each sprint has a coherent theme and deliverable
- Check for missing implementation details (NC_Dispatch functions, data structures, error handling)
- Verify testing requirements (C unit tests, Fortran tests, integration tests)
- Assess build system integration (CMake and Autotools)
- Identify dependency requirements and version constraints

### 5. Ask 3-6 numbered clarifying questions

**Focus areas:**
- Sprint organization and dependency flow
- Ambiguous requirements or missing technical details
- Error handling strategies and NetCDF error code mapping
- Testing expectations and coverage targets per sprint
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

### 6. Update roadmap with sprint breakdown

- Incorporate answers from step 5 into `docs/roadmap.md`
- Create 1-5 sprint sections with clear themes and deliverables
- Distribute work items logically across sprints
- Clarify ambiguous tasks with specific details
- Add dependency information between sprints
- Update definition of done for each sprint
- Ensure each sprint builds incrementally toward version goals

**Important:**
- **DO NOT** add proposed code changes to the roadmap
- Code can be added to individual sprint plans (next step)
- If example code is presented by the user, that may be included in the roadmap

### 7. Update related documentation (if needed)

**Only update if clarifications revealed:**
- Architecture changes → Update `docs/design.md`
- Functional requirement changes → Update `docs/prd.md`
- NC_Dispatch or UDF handler design notes → Add to `docs/design.md`

**Skip this step if:** No architectural or functional changes were clarified

### 8. Verify completion criteria

**Confirm all items are satisfied:**
- ✓ Version work is broken into 1-5 logical sprints
- ✓ Each sprint has a coherent theme and clear deliverables
- ✓ Dependencies between sprints are documented
- ✓ All sprint tasks are clearly defined
- ✓ Implementation approach is specified for each major task
- ✓ Testing requirements are identified for each sprint
- ✓ Build system integration is addressed
- ✓ Dependencies are documented
- ✓ Error handling strategy is defined
- ✓ Roadmap is updated with all clarifications

**If any criteria are missing:** Return to step 5 and ask additional questions

**Next step:** User can run `/plan <version> sprint <number>` to create detailed implementation plan for individual sprints

---

## Error Handling

**If version not found in roadmap:**
- Report: "Version {version} not found in docs/roadmap.md"
- List available versions
- Stop workflow

**If version scope is too large for 5 sprints:**
- Report: "Version {version} work is too complex for 5 sprints"
- Suggest breaking into multiple versions or reducing scope
- Ask user how to proceed

**If version has insufficient work for 1 sprint:**
- Report: "Version {version} has minimal work, consider combining with another version"
- Ask user if they want to proceed with a single sprint

**If dependencies not met:**
- Report: "Previous version dependencies not satisfied"
- List missing dependencies
- Ask user if they want to proceed anyway

**If roadmap is malformed:**
- Report specific formatting issues found
- Ask user to fix roadmap structure first
- Stop workflow
