---
description: Plan one sprint from the roadmap and create its GitHub issue
---

# Sprint Planning Workflow (Roadmap)

This workflow reads the roadmap document, selects the specified sprint for a version, asks clarifying questions, expands the sprint description in the roadmap, and creates a single GitHub issue for the sprint.

## Usage

```
/roadmap <version> sprint <number>
```

**Example:** `/roadmap v2.4.0 sprint 1`

**Output:** Updated sprint description in `docs/roadmap.md` and one GitHub issue for the sprint.

**Note:** Only one sprint is planned per invocation.

---

## Scope

**Do:**
- Read `docs/roadmap.md` to identify the sprint starting point
- Plan exactly one sprint per invocation
- Ask clarifying questions about the selected sprint
- Expand the sprint description in `docs/roadmap.md`
- Create one GitHub issue for the whole sprint
- Link the roadmap sprint to the GitHub issue

**Don't:**
- Plan multiple sprints in one run
- Create a version-level GitHub Project
- Implement code (save for `/implement` workflow)
- Modify historical versions — these are frozen

---

## Steps

### 1. Read roadmap and select one sprint

- Read `docs/roadmap.md`
- Locate the specified version and sprint number from the command
- **If the version is historical:** Report "Historical versions are not supported" and stop workflow
- **If the sprint is not found:** Report "Sprint {n} not found for version {version}" and stop workflow
- Record the selected sprint number and initial description
- Check whether the sprint already has a GitHub issue link; if so, warn that it may be re-planned

### 2. Review related documentation for context

- Check `docs/prd.md` for relevant product requirements and functional specifications
- Review `docs/design.md` for architecture and technical design
- Read any existing `docs/plan/v{version}-sprint{n}-*.md` files if present
- Check source code in `src/`, `include/`, and `test/` for existing patterns

### 3. Ask 3-6 numbered clarifying questions

**Focus areas:**
- Sprint scope and deliverables
- Ambiguous requirements or missing technical details from the roadmap
- Error handling strategies and NetCDF error code mapping
- Testing expectations and coverage targets
- Build system integration and dependency detection
- Memory management and resource cleanup
- Integration with native format libraries (NCEPLIBS-g2, libgeotiff, NASA CDF, CFITSIO)

**Question format:**
- Provide suggested answers, labeled with letters, so questions can be answered with just a letter
- Recommend an answer and provide justification for your recommendation
- Ask minimum 3 questions, maximum 6 questions

**⚠️ ASK QUESTIONS NOW**

**Wait for user responses before proceeding**

**If answers are incomplete:**
- Ask follow-up questions to clarify
- Maximum 2 rounds of questions total

### 4. Expand sprint description in the roadmap

- Incorporate answers from step 3 into the sprint description in `docs/roadmap.md`
- Add a **Clarified decisions:** section if it does not exist
- Add or update a **Detailed Plan** reference line (`See docs/plan/v{version}-sprint{n}-<topic>.md`)
- Ensure the sprint description is specific enough for implementation
- Keep descriptions at the planning level; do not add proposed code changes to the roadmap

### 5. Plan the sprint

- Build a concise implementation plan for the selected sprint only
- Include in the plan:
  - Sprint goal and scope
  - Major tasks in dependency order
  - Acceptance criteria
  - Testing requirements, specifically testing any new capabilities added by the sprint
  - Build system integration points (CMake and Autotools)
  - Dependencies and blockers
  - Documentation updates required (e.g., `docs/prd.md`, `docs/design.md`, `README.md`, Doxygen, format guides)
  - Definition of done
- Save the plan to `docs/plan/v{version}-sprint{n}-<topic>.md` if it does not already exist

### 6. Create one GitHub issue for the whole sprint

- Use `gh issue create` to create the issue; prefer the GitHub CLI over MCP tools because it is more reliable on slow networks
- Title: `v{version} Sprint {n}: <sprint theme>`
- Body must include:
  - Sprint goal and scope
  - Expanded description from the roadmap
  - Acceptance criteria
  - Testing requirements, including tests for any new capabilities
  - Documentation updates required
  - Dependencies
  - Definition of done
  - Link to detailed plan: `docs/plan/v{version}-sprint{n}-<topic>.md`
- Record the issue number returned by `gh issue create`

### 7. Update roadmap with issue link

- In `docs/roadmap.md`, add the GitHub issue number/link to the selected sprint
- Example line: `**GitHub Issue:** #123`
- Ensure the detailed plan reference is correct

### 8. Verify completion criteria

**Confirm all items are satisfied:**
- ✓ One sprint was selected from the roadmap
- ✓ Clarifying questions were asked and answered
- ✓ Sprint description in `docs/roadmap.md` was expanded
- ✓ Sprint plan was created or updated
- ✓ One GitHub issue was created for the sprint
- ✓ Roadmap links to the GitHub issue and detailed plan
- ✓ Next step is clearly stated

**If any criteria are missing:** Return to step 3 and ask additional questions

**Next step:** Run `/implement <version> sprint <sprint_number>` to implement the sprint.

---

## Error Handling

**If version not found in roadmap:**
- Report "Version {version} not found in docs/roadmap.md"
- List available versions from the roadmap
- Stop workflow

**If the version is historical:**
- Report "Historical versions are not supported"
- Stop workflow

**If the specified sprint is not found:**
- Report "Sprint {n} not found for version {version}"
- List available sprints for the version
- Stop workflow

**If all sprints are already planned:**
- Report "All sprints for {version} already have GitHub issues"
- List the existing sprint issues
- Stop workflow

**If dependencies not met:**
- Report "Previous sprint dependencies not satisfied"
- List missing dependencies
- Ask user if they want to proceed anyway

**If GitHub issue creation fails:**
- Report the `gh issue create` error output
- Suggest manual issue creation with title `v{version} Sprint {n}: <sprint theme>`
- Provide the issue body for manual setup
- Stop workflow
