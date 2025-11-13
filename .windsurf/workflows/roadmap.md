---
description: Expand sprint requirements in the roadmap document
auto_execution_mode: 3
---

# Roadmap Sprint Expansion Workflow

This workflow creates detailed sprint planning documents from roadmap entries.

## Usage

```
/roadmap <version> sprint <number>
```

Example: `/roadmap v0.4.0 sprint 3`

## Steps

1. **Extract sprint details from roadmap**
   - Read `docs/roadmap.md` and locate the specified version and sprint number
   - Extract all tasks, goals, and definition of done criteria
   - Identify dependencies on previous sprints or external systems

2. **Review related documentation for context**
   - Check `docs/prd.md` for product requirements and functional specifications
   - Review `docs/design.md` for architecture and technical design
   - Review existing sprint plans in `docs/plan/` to understand format and detail level
   - Check source code in `src/`, `include/`, and `test/` for existing patterns

3. **Analyze sprint scope and identify gaps**
   - Review each task for completeness and clarity
   - Identify missing implementation details (NC_Dispatch functions, data structures, error handling)
   - Check for missing testing requirements (C unit tests, Fortran tests, integration tests)
   - Verify build system integration (CMake and Autotools)
   - Identify dependency requirements and version constraints

4. **Ask 3-6 numbered clarifying questions**
   - Focus on ambiguous requirements or missing technical details
   - Ask about error handling strategies and NetCDF error code mapping
   - Clarify testing expectations and coverage targets
   - Question build system integration and dependency detection
   - Ask about memory management and resource cleanup
   - Verify integration with native format libraries (NCEPLIBS-g2, libgeotiff, NASA CDF)
   - Provide suggested answers, labeled with letters, so that the question can be answered with just a letter
   - Recommend an answer and provide a justification for your recommendation
   - **ASK QUESTIONS NOW**
   - **Wait for user responses before proceeding**

5. **Update roadmap with clarifications**
   - Incorporate answers from step 4 into `docs/roadmap.md`
   - Add implementation decisions to sprint section
   - Clarify ambiguous tasks with specific details
   - Add any new subtasks or requirements discovered
   - Update definition of done if needed
   - Do not add proposed code changes to the roadmap. Code can be added to the plan (next step).
   - If example code is presented by the user, that may be included in the roadmap.
   - DO NOT PUT CODE INTO THE ROADMAP, SAVE IT FOR THE PLANNING DOCUMENT

7. **Update related documentation**
   - Update `docs/design.md` if architecture or technical design details were clarified
   - Update `docs/prd.md` if functional requirements were modified
   - Add notes about NC_Dispatch implementations or UDF handler design if applicable

8. **Plan**
   - Run the "plan" workflow for this version and sprint