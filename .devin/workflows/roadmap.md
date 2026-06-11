---
description: Break version work into detailed sprint planning documents
---

# Version Roadmap Planning Workflow

This workflow breaks down a version's work into 1-5 detailed sprint planning documents.

## Usage

```
/roadmap <version> <description>
```

**Example:** `/roadmap v1.6.0 Add CF conventions for GeoTIFF metadata`

**Output:** Creates version-specific GitHub Project "NEP Version 1.6.0 - GeoTIFF metadata to CF" with sprint breakdown

**Note:** Only v1.6.0+ versions are supported. Historical versions are not updated.

---

## Scope

**Do:**
- Analyze the specified version's work and break it into 1-5 sprints
- Identify technical gaps and missing requirements across all sprints
- Create version-specific GitHub Project with sprint breakdown and task organization
- Ensure logical dependency flow between sprints

**Don't:**
- Modify other versions not specified
- Implement code (save for `/plan` workflow)
- Create more than 5 sprints for a version
- Skip dependency analysis between sprints
- Add code changes to GitHub Project (use Issues/PRs instead)
- Update historical versions (v1.5.0 and earlier) - these are frozen

---

## Project Creation Process

### Naming Convention
Version-specific projects follow the pattern: `NEP Version {version} - {brief description}`

**Examples:**
- "NEP Version 1.6.0 - GeoTIFF metadata to CF"
- "NEP Version 1.7.0 - GRIB2 write support"
- "NEP Version 1.8.0 - Performance optimization"

### Project Structure
Each version-specific project includes:
- **Organization-level project** (not repository-specific)
- **Standard fields**: Version, Sprint, Task Status, Priority, Component
- **Field values**: 
  - Version: "{version}" for all issues
  - Sprint: 1-5 for respective sprint issues
  - Task Status: Backlog → In Progress → Review → Done
  - Priority: High/Medium/Low
  - Component: Build System/UDF Handler/Tests/Documentation
- **Issues**: Main version issue + individual sprint issues

### Implementation Tools
- **GitHub CLI** (`gh project create`) for automated creation
- **GitHub API** as fallback for advanced configuration
- **Manual creation** option when automation fails

---

## Steps

### 1. Validate version and create version-specific GitHub Project

- Check if version-specific GitHub Project exists for NEP organization
- **If no project exists:** Create new organization-level GitHub Project with:
  - Project name: "NEP Version {version} - {brief description}" (e.g., "NEP Version 1.6.0 - GeoTIFF metadata to CF")
  - Owner: Intelligent-Data-Design-Inc (organization level)
  - Fields: Version (text), Sprint (number), Task Status (single select: Backlog, In Progress, Review, Done), Priority (single select: High, Medium, Low), Component (single select: Build System, UDF Handler, Tests, Documentation)
  - Add all relevant issues to the project
- Query GitHub Projects for the specified version
- **If version is v1.5.0 or earlier:** Report that historical versions are not supported and stop workflow

### 2. Extract version details

- Locate the specified version in GitHub Project
- **If version exists but has no items:** Create initial project items for the version based on:
  - User-provided version description from the command prompt
  - Known technical components and requirements
  - Standard sprint structure (file operations, metadata, data reading, etc.)
- Extract all existing work items, goals, and requirements from project items
- Identify any existing sprint breakdown from project fields
- Note incomplete or ambiguous requirements from item descriptions
- Assess total scope and complexity from linked Issues/PRs

### 3. Review related documentation for context

- Check `docs/prd.md` for product requirements and functional specifications
- Review `docs/design.md` for architecture and technical design
- Review existing sprint plans in `docs/plan/` to understand format and detail level
- Check source code in `src/`, `include/`, and `test/` for existing patterns

### 4. Analyze version scope and identify optimal sprint breakdown

- Review all work items for completeness and clarity
- Use the user-provided version description as the primary source for requirements
- Group related tasks into logical sprint units (1-5 sprints total)
- Identify dependencies between task groups
- Ensure each sprint has a coherent theme and deliverable
- Check for missing implementation details (NC_Dispatch functions, data structures, error handling)
- Verify testing requirements (C unit tests, Fortran tests, integration tests)
- Assess build system integration (CMake and Autotools)
- Identify dependency requirements and version constraints

### 5. Ask 3-6 numbered clarifying questions

**Focus areas:**
- Sprint organization and dependency flow based on user-provided description
- Ambiguous requirements or missing technical details from the initial prompt
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

- Incorporate answers from step 5 into GitHub Project
- Create 1-5 sprint sections using project fields and organization
- Distribute work items logically across sprints by updating item fields
- Clarify ambiguous tasks with specific details in item descriptions
- Add dependency information between project items
- Update definition of done for each sprint in project metadata
- Ensure each sprint builds incrementally toward version goals
- Link related Issues/PRs to project items for implementation

**Important:**
- **DO NOT** add proposed code changes to project item descriptions
- Code implementation should be tracked in separate Issues/PRs linked to project items
- If example code is presented by the user, that may be included in item descriptions

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
- ✓ GitHub Project is updated with all clarifications and sprint organization

**If any criteria are missing:** Return to step 5 and ask additional questions

**Next step:** User can run `/plan <version> sprint <number>` to create detailed implementation plan for individual sprints

---

## Error Handling

**If version not found:**
- Report "Version {version} not found in GitHub Project"
- List available versions from project
- Stop workflow

**If version is v1.5.0 or earlier:**
- Report "Historical versions (v1.5.0 and earlier) are not supported"
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

**If GitHub Project creation fails:**
- Report specific GitHub API errors for project creation
- Suggest manual project creation with the naming convention "NEP Version {version} - {brief description}"
- Provide field configuration details for manual setup
- Stop workflow

**If GitHub Project operations fail:**
- Report specific GitHub API errors
- Suggest manual project updates as fallback
- Continue with documentation updates if possible
